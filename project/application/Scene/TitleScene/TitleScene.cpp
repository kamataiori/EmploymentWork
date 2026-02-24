#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <PostEffectManager.h>
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"

static float EaseOutCubic(float t) {
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	const float u = 1.0f - t;
	return 1.0f - (u * u * u);
}

void TitleScene::Initialize()
{
	// ==============================================
	//    BaseSceneがLightを持っているため
	//    LightのInitialize()は必ず必要
	// ==============================================

	// Lightクラスのデータを初期化
	BaseScene::GetLight()->Initialize();
	BaseScene::GetLight()->GetCameraLight();
	BaseScene::GetLight()->GetDirectionalLight();
	BaseScene::GetLight()->SetDirectionalLightIntensity({ 1.0f });
	BaseScene::GetLight()->SetDirectionalLightColor({ 1.0f,1.0f,1.0f,1.0f });
	//BaseScene::GetLight()->SetDirectionalLightDirection(Normalize({ 1.0f,1.0f }));
	/*BaseScene::GetLight()->GetSpotLight();
	BaseScene::GetLight()->SetCameraPosition({ 0.0f, 1.0f, 0.0f });
	BaseScene::GetLight()->SetSpotLightColor({ 1.0f,1.0f,1.0f,1.0f });
	BaseScene::GetLight()->SetSpotLightPosition({ 10.0f,2.25f,0.0f });
	BaseScene::GetLight()->SetSpotLightIntensity({ 4.0f });*/

	// 3Dオブジェクトの初期化

	sneak = std::make_unique<Object3d>(this);
	sneak->Initialize();

	sword = std::make_unique<Object3d>(this);
	sword->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");
	ModelManager::GetInstance()->LoadModel("human/walk.gltf");
	ModelManager::GetInstance()->LoadModel("Warrior.gltf");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("skydome.obj");
	ModelManager::GetInstance()->LoadModel("sword.obj");
	sneak->SetModel("Warrior.gltf");


	// モデルにSRTを設定
	transform.scale = { 1,1,1 };
	transform.rotate = { 0.0f,3.14f,0.0f };
	transform.translate = { 0.0f,-1.0f,10.0f };
	sneak->SetTranslate(transform.translate);
	sneak->SetRotate(transform.rotate);
	sneak->SetScale(transform.scale);
	sneak->SetAnimation("Idle");

	

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,-2.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });


	// 3Dカメラの初期化
	camera1 = std::make_unique<Camera>();
	camera1->SetTranslate({ 0.0f, 1.0f, -20.0f });
	camera1->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera1->SetFarClip(2000.0f);
	// 初期角度を「今のカメラ位置」から合わせたい場合（任意）
	orbitAngle_ = 0.0f;
	orbitRadius_ = 40.0f;
	orbitHeight_ = 3.0f;
	orbitSpeed_ = -0.5f;


	// カメラのセット
	sneak->SetCamera(camera1.get());
	ground->SetCamera(camera1.get());
	sky->SetCamera(camera1.get());


	//skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(camera1.get());

	// ===== タイトル表示位置（中央よりちょい上）=====
	// 画面中央よりちょい上
	titleCenterPos_ = {
		WinApp::kClientWidth * 0.5f,
		WinApp::kClientHeight * 0.35f
	};

	// 分割用（TitleCut）
	titleTop_ = std::make_unique<Sprite>();
	titleTop_->Initialize("Resources/Title.png");
	titleTop_->SetAnchorPoint({ 0.5f, 0.5f });

	titleBottom_ = std::make_unique<Sprite>();
	titleBottom_->Initialize("Resources/Title.png");
	titleBottom_->SetAnchorPoint({ 0.5f, 0.5f });

	// 画像サイズ取得（必ずメタデータから）
	const auto& md = TextureManager::GetInstance()->GetMetaData("Resources/Title.png");
	titleTexSize_ = { (float)md.width, (float)md.height };

	const float halfH = titleTexSize_.y * 0.5f;

	// 上半分（UV）
	titleTop_->SetTextureLeftTop({ 0.0f, 0.0f });
	titleTop_->SetTextureSize({ titleTexSize_.x, halfH });

	// 下半分（UV）
	titleBottom_->SetTextureLeftTop({ 0.0f, halfH });
	titleBottom_->SetTextureSize({ titleTexSize_.x, halfH });

	// ★ここが重要：表示サイズも半分にする（伸びなくなる）
	titleTop_->SetSize({ titleTexSize_.x, halfH });
	titleBottom_->SetSize({ titleTexSize_.x, halfH });

	// ★位置は ± halfH/2（= ±45）にする（今の ±22.5 は半分ズレ）
	titleTop_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f });
	titleBottom_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f });

	phase_ = TitlePhase::Idle;
	requestedShutter_ = false;
	titleCutTimer_ = 0.0f;
}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{
	// ===== タイトルスプライト更新（フェーズで切替）=====
	titleTop_->Update();
	titleBottom_->Update();
	
	// 各3Dオブジェクトの更新

	sword->SetParentJoint(sneak.get(), "Fist.R");

	sneak->SetTranslate(transform.translate);
	sneak->SetRotate(transform.rotate);
	sneak->SetScale(transform.scale);

	sneak->Update();

	// カメラの更新
	// ===== オービットカメラ：sneak を中心に回す =====
	{
		// Δt（TimeManager があるならそっちを使うのがおすすめ）
		const float dt = TimeManager::GetInstance()->GetDeltaTime();

		orbitAngle_ += orbitSpeed_ * dt;

		// 追従ターゲット（sneakの位置 + 少し上を見る）
		const Vector3 target = transform.translate + orbitTargetOffset_;

		// カメラ位置（XZで円運動）
		const float x = target.x + std::sin(orbitAngle_) * orbitRadius_;
		const float z = target.z + std::cos(orbitAngle_) * orbitRadius_;
		const float y = target.y + orbitHeight_;

		camera1->SetTranslate({ x, y, z });

		// カメラを常にターゲットへ向ける（LookAt的に回転を計算）
		// エンジンが rotate(yaw,pitch,roll) を使う前提で計算
		Vector3 to = target - Vector3{ x, y, z };
		to = Normalize(to);

		const float yaw = std::atan2(to.x, to.z);            // Y回転
		const float pitch = std::atan2(-to.y, std::sqrt(to.x * to.x + to.z * to.z)); // X回転

		camera1->SetRotate({ pitch, yaw, 0.0f });
	}

	camera1->Update();
	ground->Update();
	sky->Update();
	//skybox->Update();

	// デバッグ
	//Debug();

	// ===== 入力（Idleのときだけ）=====
	if (!SceneManager::GetInstance()->IsTransitioning()) {
		if (phase_ == TitlePhase::Idle) {
			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				StartTitleCut();
			}
		}
	}

	// ===== TitleCut 更新 =====
	if (phase_ == TitlePhase::TitleCut) {
		const float dt = TimeManager::GetInstance()->GetDeltaTime();
		UpdateTitleCut(dt);
	}

}

void TitleScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の背景描画
	// ================================================


	// ================================================
	// ここまでSprite個々の背景描画
	// ================================================
}

void TitleScene::Draw()
{
	//skybox->Draw();

	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	//sky->Draw();
	ground->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	sneak->Draw();

	// ================================================
	// ここまでアニメーションオブジェクトの個々の描画
	// ================================================

	// ================================================
	// ここからDrawLine個々の描画
	// ================================================

	

	// ================================================
	// ここまでDrawLine個々の描画
	// ================================================
}

void TitleScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================

	// ===== タイトル描画（フェーズで切替）=====
	titleTop_->Draw();
	titleBottom_->Draw();

	

	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void TitleScene::Debug()
{
#ifdef USE_IMGUI

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など Scene UI
	//BaseScene::ShowFPS();


	
#endif
}

void TitleScene::StartTitleCut()
{
	phase_ = TitlePhase::TitleCut;
	titleCutTimer_ = 0.0f;
	requestedShutter_ = false;

	// キャラを攻撃アニメへ
	sneak->SetAnimation("Attack02");

	// 演出中はカメラ回転止めたいなら
	orbitSpeed_ = 0.0f;

	// 分割スプライトを初期位置に戻す
	const float halfH = titleTexSize_.y * 0.5f; // 90
	titleTop_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y - halfH * 0.25f });
	titleBottom_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y + halfH * 0.25f });

}

void TitleScene::UpdateTitleCut(float dt)
{
	titleCutTimer_ += dt;

	const float t = titleCutTimer_ / titleCutDuration_;
	const float e = EaseOutCubic(t);

	const float dx = titleCutDistance_ * e;
	const float dy = titleCutExtraY_ * e;

	const float halfH = titleTexSize_.y * 0.5f; // 90
	const Vector2 topBase = { titleCenterPos_.x, titleCenterPos_.y - halfH * 0.25f };
	const Vector2 botBase = { titleCenterPos_.x, titleCenterPos_.y + halfH * 0.25f };

	// 上半分を左上へ、下半分を右下へ（裂けた感じ）
	titleTop_->SetPosition({ topBase.x - dx, topBase.y - dy });
	titleBottom_->SetPosition({ botBase.x + dx, botBase.y + dy });

	// 演出終了 → Shutter
	if (!requestedShutter_ && titleCutTimer_ >= titleCutDuration_) {
		requestedShutter_ = true;

		PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);

		TransitionRequest req{};
		req.type = TransitionType::Shutter;
		req.fadeOutSec = 2.0f;  // 閉じる
		req.fadeInSec = 2.5f;  // 開く

		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}
}

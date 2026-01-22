#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <PostEffectManager.h>

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

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");
	ModelManager::GetInstance()->LoadModel("human/walk.gltf");
	ModelManager::GetInstance()->LoadModel("Warrior.gltf");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("skydome.obj");
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

	
	fade_ = std::make_unique<Fade>();
	fade_->Initialize();

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(camera1.get());

	title = std::make_unique<Sprite>();
	title->Initialize("Resources/title.png");
}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{
	title->Update();
	
	// 各3Dオブジェクトの更新
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
	skybox->Update();

	// デバッグ
	//Debug();

	// フェード処理
	if (fade_) {
		fade_->Update();

		// フェードアウト完了後にシーン遷移
		if (!nextSceneName_.empty() && !fade_->IsFinish()) {
			SceneManager::GetInstance()->ChangeScene(nextSceneName_);
			nextSceneName_.clear(); // 一度きりでリセット
		}
	}

	// キー入力でフェード開始（シーン遷移予約）
	if (!fade_->IsActive()) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			nextSceneName_ = "TUTORIAL";
		}
		/*if (Input::GetInstance()->TriggerKey(DIK_U)) {
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			nextSceneName_ = "Unity";
		}
		if (Input::GetInstance()->TriggerKey(DIK_P)) {
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			nextSceneName_ = "PARTICLE";
		}*/
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
	sky->Draw();
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

	//title->Draw();

	if (fade_) {
		fade_->Draw();
	}


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

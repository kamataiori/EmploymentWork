#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include <algorithm>
#include <cmath>


// ---------- 小ユーティリティ ----------
namespace {
	inline float EaseInOutCubic01(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return (t < 0.5f) ? 4.0f * t * t * t
			: 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
	}
	inline Vector2 LerpVec2f(const Vector2& a, const Vector2& b, float t) {
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
	}
}


void GamePlayScene::Initialize()
{
	// ライト
	// Lightクラスのデータを初期化
	BaseScene::GetLight()->Initialize();
	BaseScene::GetLight()->GetCameraLight();
	BaseScene::GetLight()->GetDirectionalLight();
	BaseScene::GetLight()->SetDirectionalLightIntensity({ 1.0f });
	BaseScene::GetLight()->SetDirectionalLightColor({ 1.0f,1.0f,1.0f,1.0f });

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("Warrior.gltf");
	ModelManager::GetInstance()->LoadModel("Rogue.gltf");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("stage.obj");
	ModelManager::GetInstance()->LoadModel("skydome.obj");

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	player_ = std::make_unique<Player>(this);
	enemy_ = std::make_unique<Enemy>(this);

	followCamera = std::make_unique<FollowCamera>(player_->GetCurrentCharacter(), 30.0f, 1.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize(followCamera.get());
	/*followCamera->SetTarget(player_->Get());*/

	enemy_->Initialize();
	enemy_->SetCamera(followCamera.get());

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,-1.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	skybox->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	sky->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage"); // Resources/Json/stage.json を読み込む
	stage_->SetCamera(followCamera.get());



	collisionMAnager_ = std::make_unique<CollisionManager>();

	AddRightDockWindow(kWindowName_MonsterControl);

	// ===== 逆シャッターのセットアップ =====
	const float screenW = 1280.0f;
	const float screenH = 720.0f;

	shutterTop_ = std::make_unique<Sprite>();
	shutterBottom_ = std::make_unique<Sprite>();
	shutterTop_->Initialize("Resources/Black.png");
	shutterBottom_->Initialize("Resources/Black.png");

	// アンカー（Title の閉演出と対にする）
	shutterTop_->SetAnchorPoint({ 0.5f, 1.0f }); // 下辺基準
	shutterBottom_->SetAnchorPoint({ 0.5f, 0.0f }); // 上辺基準

	// 画面を覆えるサイズ（片側で半分＋少し重ね気味）
	shutterTop_->SetSize({ screenW, screenH * 0.55f });
	shutterBottom_->SetSize({ screenW, screenH * 0.55f });

	// 位置定義：Title と正反対
	shutterOpen_.topStart = { screenW * 0.5f, 0.0f };        // 画面上外（開き終わり）
	shutterOpen_.botStart = { screenW * 0.5f, screenH };     // 画面下外（開き終わり）
	shutterOpen_.topEnd = { screenW * 0.5f, screenH * 0.5f }; // 中央で閉（開始）
	shutterOpen_.botEnd = { screenW * 0.5f, screenH * 0.5f };

	// 初期配置：まずは中央でぴったり閉じている（真っ黒）
	shutterTop_->SetPosition(shutterOpen_.topEnd);
	shutterBottom_->SetPosition(shutterOpen_.botEnd);

	// シーン開始と同時に “中央で少しホールド → 開く”
	BeginShutterOpen(/*duration=*/1.0f, /*holdAtCenter=*/0.25f);
}

void GamePlayScene::Finalize()
{

}

void GamePlayScene::Update()
{
	// ===== 逆シャッター進行 =====
	if (shutterOpen_.active) {
		const float dt = 1.0f / 60.0f;

		// 開き始める前に少し“黒で間”をおく
		if (shutterOpen_.holdTimer < shutterOpen_.holdSec) {
			shutterOpen_.holdTimer += dt;

			// 常に中央位置（真っ黒のまま）
			shutterTop_->SetPosition(shutterOpen_.topEnd);
			shutterBottom_->SetPosition(shutterOpen_.botEnd);
		}
		else {
			// 進捗を増やして外側へ移動（中央→画面外）
			shutterOpen_.t = std::min(1.0f,
				shutterOpen_.t + dt / (std::max)(0.001f, shutterOpen_.duration));
			float e = EaseInOutCubic01(shutterOpen_.t);
			Vector2 topPos = LerpVec2f(shutterOpen_.topEnd, shutterOpen_.topStart, e);
			Vector2 botPos = LerpVec2f(shutterOpen_.botEnd, shutterOpen_.botStart, e);


			shutterTop_->SetPosition(topPos);
			shutterBottom_->SetPosition(botPos);

			if (shutterOpen_.t >= 1.0f) {
				// 完全に画面外へ出たら演出終了（非アクティブ）
				shutterOpen_.active = false;
			}
		}

		// スプライト内部更新
		shutterTop_->Update();
		shutterBottom_->Update();
	}


	// 各3Dオブジェクトの更新
	stage_->Update();
	skybox->Update();
	ground->Update();
	sky->Update();
	player_->Update();
	enemy_->Update();

	// カメラの更新
	camera1->Update();
	followCamera->Update();

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}


	collisionMAnager_->RegisterCollider(player_->Get());
	//collisionMAnager_->RegisterCollider(enemy_.get());
	/*if (player_->GetBullet()) {
		auto bullet = player_->GetBullet();
		collisionMAnager_->RegisterCollider(bullet);
	}*/
	/*for (const auto& areaAttack : enemy_->GetAreaAttacks()) {
		collisionMAnager_->RegisterCollider(areaAttack.get());
	}
	for (const auto& bulletAttack : enemy_->GetAttackBulets()) {
		collisionMAnager_->RegisterCollider(bulletAttack.get());
	}*/


	// 衝突判定と応答
	CheckAllColisions();

	Debug();

	if (Input::GetInstance()->TriggerKey(DIK_T)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	if (Input::GetInstance()->TriggerKey(DIK_U)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("Unity");
	}
	
}

void GamePlayScene::BackGroundDraw()
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

void GamePlayScene::Draw()
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
	player_->Draw();
	enemy_->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	player_->SkinningDraw();
	enemy_->DrawModel();
	

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

void GamePlayScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================

	// === 画面最前面で逆シャッターを描く ===
	if (shutterOpen_.active) {
		shutterTop_->Draw();
		shutterBottom_->Draw();
	}

	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	player_->ParticlDraw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void GamePlayScene::Debug()
{
#ifdef _DEBUG

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など

	

#endif
}

void GamePlayScene::CheckAllColisions()
{
	collisionMAnager_->CheckAllCollisions();
}

void GamePlayScene::BeginShutterOpen(float duration, float holdAtCenter)
{
	shutterOpen_.active = true;
	shutterOpen_.t = 0.0f;
	shutterOpen_.duration = (std::max)(0.03f, duration);     // マクロ衝突回避で (std::max)
	shutterOpen_.holdSec = (std::max)(0.0f, holdAtCenter); // 同上
	shutterOpen_.holdTimer = 0.0f;

	// 中央で塞いだ状態から開始（真っ黒）
	shutterTop_->SetPosition(shutterOpen_.topEnd);
	shutterBottom_->SetPosition(shutterOpen_.botEnd);
}

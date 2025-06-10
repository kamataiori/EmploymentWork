#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>

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
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("stage.obj");

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	player_ = std::make_unique<Player>(this);
	player_->Initialize();

	followCamera = std::make_unique<FollowCamera>(player_.get(), 30.0f, 1.0f);
	followCamera->SetFarClip(2000.0f);

	enemy_ = std::make_unique<Enemy>(this);
	enemy_->Initialize();

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,-1.0f,0.0f });

	skybox->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	player_->SetCamera(followCamera.get());
	enemy_->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage"); // Resources/Json/stage.json を読み込む
	stage_->SetCamera(followCamera.get());



	collisionMAnager_ = std::make_unique<CollisionManager>();
	/*collisionMAnager_->RegisterCollider(player_.get());
	collisionMAnager_->RegisterCollider(enemy_.get());
	if (auto bullet = player_->GetBullet()) {
		collisionMAnager_->RegisterCollider(bullet);
	}*/


	AddRightDockWindow(kWindowName_MonsterControl);

}

void GamePlayScene::Finalize()
{

}

void GamePlayScene::Update()
{
	// 各3Dオブジェクトの更新
	stage_->Update();
	skybox->Update();
	ground->Update();
	player_->Update();
	enemy_->Update();

	// カメラの更新
	camera1->Update();
	followCamera->Update();

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}

	ImGui::Text("bullet_ exists: %s", player_->GetBullet() ? "Yes" : "No");

	collisionMAnager_->RegisterCollider(player_.get());
	collisionMAnager_->RegisterCollider(enemy_.get());
	if (player_->GetBullet()) {
		auto bullet = player_->GetBullet();
		collisionMAnager_->RegisterCollider(bullet);
	}


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

	skybox->Draw();

	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	stage_->Draw();
	ground->Draw();
	player_->Draw();
	enemy_->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
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


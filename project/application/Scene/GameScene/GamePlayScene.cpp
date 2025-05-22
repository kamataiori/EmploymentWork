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

	AddRightDockWindow(kWindowName_MonsterControl);

	// playerの初期化
	player_ = std::make_unique<Player>(this);
	player_->Initialize();

	// followCameraの初期化
	followCamera = std::make_unique<FollowCamera>(player_.get(), 10.0f, -1.5f, -4.0f);
	followCamera->SetFovY(70.0f * (3.14159265f / 180.0f));

	// Objectをカメラにセット
	player_->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(followCamera.get());

	// 当たり判定の初期化・セット
	collisionManager_ = std::make_unique<CollisionManager>();
	collisionManager_->RegisterCollider(player_.get());

}

void GamePlayScene::Finalize()
{
	
}

void GamePlayScene::Update()
{
	// playerの更新
	player_->Update();

	// カメラの更新
	followCamera->Update();

	skybox->Update();

	// 衝突判定と応答
	CheckAllColisions();

	Debug();
	followCamera->Debug();


	if (Input::GetInstance()->TriggerKey(DIK_H)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	if (Input::GetInstance()->TriggerKey(DIK_U)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("Unity");
	}

	DrawLine::GetInstance()->Update();
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
	player_->Draw();
	player_->DrawBullets();


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

	player_->Draw2D();

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

	

#endif
}

void GamePlayScene::CheckAllColisions()
{
	collisionManager_->CheckAllCollisions();
}

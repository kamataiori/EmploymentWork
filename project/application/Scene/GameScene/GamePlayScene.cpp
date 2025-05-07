#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"

void GamePlayScene::Initialize()
{
	
	// 3Dオブジェクトの初期化
	ModelManager::GetInstance()->LoadModel("skydome.obj");

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	camera2->SetTranslate({ 5.0f, 1.0f, -40.0f });

	player_ = std::make_unique<Player>(this);
	player_->Initialize();

	followCamera = std::make_unique<FollowCamera>(player_.get(), 20.0f, 0.0f);

	skydome_ = std::make_unique<Object3d>(this);
	skydome_->Initialize();
	skydome_->SetModel("skydome.obj");

	heel_ = std::make_unique<Sprite>();
	heel_->Initialize("Resources/heel.png");

	//srand((unsigned int)time(nullptr)); // 乱数初期化

	// 乱数初期化（1回のみ）
	srand((unsigned int)time(nullptr));

	// heelEffects_ 配列の初期化
	heelEffects_.clear();
	for (int i = 0; i < kHeelSpriteCount; ++i) {
		HeelEffect effect;
		effect.sprite = std::make_unique<Sprite>();
		effect.sprite->Initialize("Resources/heel.png");

		effect.state = HeelEffectState::Waiting;
		effect.scale = 0.0f;
		effect.lifetime = 0.0f;

		// 出現までの時間をランダムにずらす（最大1秒）
		effect.respawnTimer = static_cast<float>(rand()) / RAND_MAX * kHeelRespawnTime;

		// 終了までの寿命もランダム（0.6〜1.2秒など）
		effect.lifetimeLimit = 0.6f + static_cast<float>(rand()) / RAND_MAX * 0.6f;

		heelEffects_.emplace_back(std::move(effect));
	}


	player_->SetCamera(followCamera.get());

	enemy_ = std::make_unique<Enemy>(this);
	enemy_->Initialize();

	enemy_->SetCamera(followCamera.get());

	skydome_->SetCamera(followCamera.get());

	DrawLine::GetInstance()->SetCamera(followCamera.get());

	// ライト
	// Lightクラスのデータを初期化
	BaseScene::GetLight()->Initialize();
	BaseScene::GetLight()->GetCameraLight();
	BaseScene::GetLight()->GetDirectionalLight();
	BaseScene::GetLight()->SetDirectionalLightIntensity({ 1.0f });
	BaseScene::GetLight()->SetDirectionalLightColor({ 1.0f,1.0f,1.0f,1.0f });
	//BaseScene::GetLight()->SetDirectionalLightDirection(Normalize({ 1.0f,1.0f }));
	BaseScene::GetLight()->GetSpotLight();
	BaseScene::GetLight()->SetCameraPosition({ 0.0f, 1.0f, 0.0f });
	BaseScene::GetLight()->SetSpotLightColor({ 1.0f,1.0f,1.0f,1.0f });
	BaseScene::GetLight()->SetSpotLightPosition({ 10.0f,2.25f,0.0f });
	BaseScene::GetLight()->SetSpotLightIntensity({ 4.0f });

	collisionMAnager_ = std::make_unique<CollisionManager>();
	collisionMAnager_->RegisterCollider(player_.get());
	collisionMAnager_->RegisterCollider(enemy_.get());

	AddRightDockWindow(kWindowName_MonsterControl);
}

void GamePlayScene::Finalize()
{
	
}

void GamePlayScene::Update()
{
	

	player_->Update();
	enemy_->Update();
	skydome_->Update();

	for (auto& effect : heelEffects_) {
		switch (effect.state) {
		case HeelEffectState::Waiting:
			effect.respawnTimer += 1.0f / 60.0f;
			if (effect.respawnTimer >= kHeelRespawnTime) {
				Vector2 center = { 1280 / 2.0f, 720 / 2.0f };
				Vector2 pos;
				int tryCount = 10;
				do {
					float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * (float)M_PI;
					float radius = kHeelMinRadius + static_cast<float>(rand()) / RAND_MAX * (kHeelMaxRadius - kHeelMinRadius);

					Vector2 offset = {
						cosf(angle) * radius,
						sinf(angle) * radius
					};
					pos = Add(center, offset);
				} while (!IsFarEnough(pos, heelEffects_, 64.0f) && --tryCount > 0);

				effect.position = pos;
				effect.scale = 0.0f;
				effect.lifetime = 0.0f;
				effect.state = HeelEffectState::Growing;
				effect.respawnTimer = 0.0f;
			}
			break;

		case HeelEffectState::Growing:
			effect.scale += kHeelGrowSpeed;
			effect.lifetime += 1.0f / 60.0f;

			if (effect.lifetime >= effect.lifetimeLimit) {
				effect.state = HeelEffectState::Inactive;
				effect.scale = 0.0f;
				effect.lifetime = 0.0f;
				effect.respawnTimer = 0.0f;
			}
			break;


		case HeelEffectState::Inactive:
			effect.state = HeelEffectState::Waiting; // すぐ次へ
			break;
		}

		// スプライトに反映
		effect.sprite->SetPosition(effect.position);
		effect.sprite->SetSize({ 64.0f * effect.scale, 64.0f * effect.scale });
		effect.sprite->SetColor({ 1, 1, 1, effect.state == HeelEffectState::Growing ? 1.0f : 0.0f });
		effect.sprite->Update();
	}




	// カメラの更新
	followCamera->Update();
	camera1->Update();
	Vector3 cameraRotate = camera2->GetRotate();
	cameraRotate.x = 0.0f;
	cameraRotate.y += 0.1f;
	cameraRotate.z = 0.0f;
	camera2->SetRotate(cameraRotate);
	camera2->Update();

	

	// 衝突判定と応答
	CheckAllColisions();

	Debug();

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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
	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	skydome_->Draw();
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

	// 回復エフェクト（＋アイコン）を円形に配置して描画
	for (auto& effect : heelEffects_) {
		if (effect.state == HeelEffectState::Growing) {
			effect.sprite->Draw();
		}
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

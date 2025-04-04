#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"

void GamePlayScene::Initialize()
{
	//-----Spriteの初期化-----

	for (uint32_t i = 0; i < 6; ++i)
	{
		// スプライトを初期化
		auto sprite = std::make_unique<Sprite>();

		if (i % 2 == 0) {
			// 0, 2, 4 は "Resources/uvChecker.png"
			sprite->Initialize("Resources/uvChecker.png");
		}
		else {
			// 1, 3, 5 は "Resources/monsterBall.png"
			sprite->Initialize("Resources/monsterBall.png");
		}

		sprites.push_back(std::move(sprite));
	}

	monsterBall = std::make_unique<Sprite>();
	monsterBall->Initialize("Resources/monsterBall.png");
	MonsterPosition = monsterBall->GetPosition();
	MonsterPosition = { 100.0f,100.0f };

	// 3Dオブジェクトの初期化
	plane = std::make_unique<Object3d>(this);
	axis = std::make_unique<Object3d>(this);

	plane->Initialize();
	axis->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	plane->SetModel("uvChecker.gltf");
	axis->SetModel("axis.obj");

	// モデルにSRTを設定
	plane->SetScale({ 1.0f, 1.0f, 1.0f });
	plane->SetRotate({ 0.0f, 3.14f, 0.0f });
	plane->SetTranslate({ -2.0f, 0.0f, 0.0f });

	axis->SetScale({ 1.0f, 1.0f, 1.0f });
	axis->SetRotate({ 0.0f, 0.0f, 0.0f });
	axis->SetTranslate({ 2.0f, 0.0f, 0.0f });

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	camera2->SetTranslate({ 5.0f, 1.0f, -40.0f });

	// カメラのセット
	plane->SetCamera(camera1.get());
	axis->SetCamera(camera1.get());
	particle->SetCamera(camera1.get());

	// Audioの初期化
	audio->Initialize();
	sound = audio->SoundLoadWave("Resources/fanfare.wav");
	//audio->SoundPlayLoopWave(audio->GetXAudio2().Get(), sound);
	//audio->SoundPlayWave(audio->GetXAudio2().Get(), sound);
	isAudio = false;


	player_ = std::make_unique<Player>(this);
	player_->Initialize();

	player_->SetCamera(camera1.get());

	enemy_ = std::make_unique<Enemy>(this);
	enemy_->Initialize();

	enemy_->SetCamera(camera1.get());

	DrawLine::GetInstance()->SetCamera(camera1.get());
	// 円錐の初期化
	cone.baseCenter = { 0.0f, -3.5f, 0.0f }; // 底面の中心
	cone.tip = { 0.0f, -1.0f, 0.0f };        // 頂点（先端）
	cone.radius = 1.0f;                   // 底面半径
	cone.color = static_cast<int>(Color::WHITE); // 色
	cone.segments = 16;                   // 円周の分割数

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


	particle->Initialize();
	particle->CreateParticleGroup("particle", "Resources/particleTest.png",ParticleManager::BlendMode::kBlendModeAdd);
	//particle->CreateParticleGroup("particle2", "Resources/circle.png", ParticleManager::BlendMode::kBlendModeAdd,{32.0f,32.0f});
	// ParticleEmitterの初期化
	auto emitter = std::make_unique<ParticleEmitter>(particle.get(), "particle", Transform{ {0.0f, 0.0f, -4.0f} }, 10, 0.5f,true);
	emitters.push_back(std::move(emitter));

	collisionMAnager_ = std::make_unique<CollisionManager>();
	collisionMAnager_->RegisterCollider(player_.get());
	collisionMAnager_->RegisterCollider(enemy_.get());
}

void GamePlayScene::Finalize()
{
	// 音声データの解放
	audio->SoundUnload(&sound);

	// Audioの終了処理
	audio->Finalize();
}

void GamePlayScene::Update()
{
	// 各3Dオブジェクトの更新
	plane->Update();
	axis->Update();

	player_->Update();
	enemy_->Update();

	// カメラの更新
	camera1->Update();
	Vector3 cameraRotate = camera2->GetRotate();
	cameraRotate.x = 0.0f;
	cameraRotate.y += 0.1f;
	cameraRotate.z = 0.0f;
	camera2->SetRotate(cameraRotate);
	camera2->Update();

	// モンスターボール
	monsterBall->SetPosition(MonsterPosition);
	monsterBall->SetSize({ 100.0f,100.0f });
	monsterBall->Update();

	ImGui::Begin("monsterBall");
	ImGui::DragFloat2("transformation", &MonsterPosition.x);
	ImGui::End();

	// 各スプライトの更新処理
	for (size_t i = 0; i < sprites.size(); ++i) {
		auto& sprite = sprites[i];

		// X座標を設定
		Vector2 position = sprite->GetPosition();
		position.x = initialX + i * offsetX;  // X座標をずらす
		position.y = 100.0f;
		sprite->SetPosition(position);

		// 角度を変化させる
		float rotation = sprite->GetRotation();
		rotation += 0.08f;
		sprite->SetRotation(rotation);

		// 色を変化させる
		Vector4 color = sprite->GetColor();
		color.x += 0.01f;
		if (color.x > 1.0f) {
			color.x -= 1.0f;
		}
		sprite->SetColor(color);

		// スプライトのサイズを設定
		sprite->SetSize(initialSize);
		sprite->SetAnchorPoint({ 0.5f, 0.5f });
		sprite->SetFlipX(false);
		sprite->SetFlipY(false);

		// テクスチャ範囲指定
		if (i % 2 == 0) {
			sprite->SetTextureLeftTop({ 0.0f, 0.0f });
			sprite->SetTextureSize({ 64.0f, 64.0f });
		}
		else {
			sprite->SetTextureLeftTop({ 600.0f, 280.0f });
			sprite->SetTextureSize({ 200.0f, 200.0f });
		}

		// 各スプライトを更新
		sprite->Update();
	}

	// 衝突判定と応答
	CheckAllColisions();

	/*ImGui::Begin("light");
	ImGui::DragFloat3("transform", &BaseScene::GetLight()->cameraLightData->worldPosition.x, 0.01f);
	ImGui::DragFloat3("DirectionalDirection", &BaseScene::GetLight()->directionalLightData->direction.x, 0.01f);
	ImGui::DragFloat("DirectionalIntensity", &BaseScene::GetLight()->directionalLightData->intensity, 0.01f);
	ImGui::DragFloat3("SpotPosition", &BaseScene::GetLight()->spotLightData->position.x, 0.01f);
	ImGui::DragFloat("SpotIntensity", &BaseScene::GetLight()->spotLightData->intensity, 0.01f);
	ImGui::End();*/

	// 音声再生を無限ループで呼び出す
	//audio->SoundPlayLoopWave(audio->GetXAudio2().Get(), sound);
    //audio->SoundPlayWave(audio->GetXAudio2().Get(), sound);

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	//particle->Emit("particle",/*plane->GetTranslate()*/ { 0.0f,0.0f,-4.0f }, 10);
	for (auto& emitter : emitters)
	{
		emitter->Update();
	}
	particle->Update();

	DrawLine::GetInstance()->Update();

	// 円錐の調整
	ImGui::Begin("Cone Control");
	ImGui::DragFloat3("Base Center", &cone.baseCenter.x, 0.1f); // 底面中心
	ImGui::DragFloat3("Tip", &cone.tip.x, 0.1f);               // 頂点（先端）
	ImGui::DragFloat("Radius", &cone.radius, 0.1f, 0.1f, 10.0f); // 半径
	ImGui::DragInt("Segments", &cone.segments, 1, 3, 64);       // 円周分割数
	ImGui::End();
}

void GamePlayScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の背景描画
	// ================================================

	monsterBall->Draw();

	// スプライトを描画する
	for (const auto& sprite : sprites) {
		sprite->Draw();
	}

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
	//plane->Draw();
	player_->Draw();
	enemy_->Draw();
	axis->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	// ================================================
	// ここからDrawLine個々の描画
	// ================================================

	/*DrawLine::GetInstance()->AddLine(
		{ 0.0f, 0.0f, 0.0f },
		{ 0.5f, 0.5f, 0.0f },
		Color::WHITE,
		Color::WHITE
	);*/

	// 円錐を描画
	DrawLine::GetInstance()->DrawCone(cone);

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

	particle->Draw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void GamePlayScene::CheckAllColisions()
{
	collisionMAnager_->CheckAllCollisions();
}

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

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	player_ = std::make_unique<Player>(this);
	player_->Initialize();

	followCamera = std::make_unique<FollowCamera>(player_.get(), 30.0f, 1.0f);

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


	collisionMAnager_ = std::make_unique<CollisionManager>();
	/*collisionMAnager_->RegisterCollider(player_.get());
	collisionMAnager_->RegisterCollider(enemy_.get());
	if (auto bullet = player_->GetBullet()) {
		collisionMAnager_->RegisterCollider(bullet);
	}*/


	AddRightDockWindow(kWindowName_MonsterControl);


	// ===== パーティクル初期化 =====
	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->CreateParticleGroup(groupName, "Resources/smoke.png", ParticleManager::BlendMode::kBlendModeAdd);
	particle->SetCamera(followCamera.get());

	emitter = std::make_unique<ParticleEmitter>();
	emitter->Initialize(particle.get(), groupName, emitterTransform, config);
	emitter->SetTransform(emitterTransform);
	emitter->SetUseRandom(useRandomPosition);  // ランダム位置設定を反映

	particle->SetColorToGroup(groupName, particleColor);
	particle->SetVelocityToGroup(groupName, particleVelocity);
	particle->SetLifeTimeToGroup(groupName, particleLifeTime);
	particle->SetCurrentTimeToGroup(groupName, particleStartTime);

	// ===== Planeのコピー：もう一つのパーティクル初期化 =====
	planeParticle2->Initialize(ParticleManager::VertexDataType::Plane);
	planeParticle2->CreateParticleGroup(groupName2, "Resources/smoke.png", ParticleManager::BlendMode::kBlendModeNormal);
	planeParticle2->SetCamera(followCamera.get());

	emitter2 = std::make_unique<ParticleEmitter>();
	emitter2->Initialize(planeParticle2.get(), groupName2, emitterTransform,
		EmitterConfig{ ShapeType::Plane, 20, 1.0f, false });
	emitter2->SetTransform(emitterTransform);
	emitter2->SetUseRandom(useRandomPosition);

	planeParticle2->SetColorToGroup(groupName2, { 0.0f, 0.0f, 0.0f, 0.2f });
	planeParticle2->SetVelocityToGroup(groupName2, particleVelocity);
	planeParticle2->SetLifeTimeToGroup(groupName2, 2.2f);
	planeParticle2->SetCurrentTimeToGroup(groupName2, particleStartTime);



	// ===== リング用パーティクルマネージャ初期化 =====
	ringParticle->Initialize(ParticleManager::VertexDataType::Ring);
	ringParticle->SetCamera(followCamera.get());

	// 1枚目: ビルボードON
	ringParticle->SetUseBillboard(true);
	ringParticle->CreateParticleGroup("ring_billboard", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	Transform t0;
	t0.translate = { 0.0f, 0.0f, 0.0f };
	t0.scale = { 2.0f, 2.0f, 2.0f };
	t0.rotate = { 0.0f, 0.0f, 0.0f };
	auto ring0 = std::make_unique<ParticleEmitter>();
	ring0->Initialize(ringParticle.get(), "ring_billboard", t0, EmitterConfig{ ShapeType::Ring, 1, 1.0f, false });
	ring0->SetTransform(t0);
	ring0->SetUseRandom(false);
	ringExplosions.push_back(std::move(ring0));

	// 2枚目: ビルボードOFF
	ringParticle->SetUseBillboard(false);
	ringParticle->CreateParticleGroup("ring_static", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);

	Transform t1;
	t1.translate = { 0.0f, 0.0f, 0.0f };
	t1.scale = { 2.0f, 2.0f, 2.0f };
	t1.rotate = { 3.1415f / 3.0f, 0.0f, 0.0f };
	auto ring1 = std::make_unique<ParticleEmitter>();
	ring1->Initialize(ringParticle.get(), "ring_static", t1, EmitterConfig{ ShapeType::Ring, 1, 1.0f, false });
	ring1->SetTransform(t1);
	ring1->SetUseRandom(false);
	ringExplosions.push_back(std::move(ring1));

	// ===== Primitive パーティクル初期化（閃光エフェクト） =====
	primitiveParticle->Initialize(ParticleManager::VertexDataType::Plane);
	primitiveParticle->SetCamera(followCamera.get());

	for (int i = 0; i < 4; ++i) {
		std::string groupName = "burst_line_" + std::to_string(i);
		primitiveParticle->CreateParticleGroup(groupName, "Resources/circle2.png", ParticleManager::BlendMode::kBlendModeAdd);

		auto emitter = std::make_unique<ParticleEmitter>();

		// time を仮に初期0とする（初期時点では sin の引数 = i）
		float time = 0.0f;
		float scaleY = 4.5f + 4.5f * std::sin(time + i);  // 初期スケール
		float rotationZ = 3.1415f * 0.25f * std::sin(time + i * 0.5f);  // 初期回転Z

		Transform t;
		t.translate = { 0.0f, 0.0f, 0.0f };
		t.scale = { 1.1f, scaleY, 1.0f };  // 細く縦長
		t.rotate = {
			0.0f,
			3.1415f * 2.0f * i / 8.0f,
			rotationZ
		};

		emitter->Initialize(primitiveParticle.get(), groupName, t, EmitterConfig{
			ShapeType::Primitive,
			1,
			1.0f,
			false
			});
		emitter->SetTransform(t);
		emitter->SetUseRandom(false);

		primitiveEmitters.push_back(std::move(emitter));
	}


}

void GamePlayScene::Finalize()
{

}

void GamePlayScene::Update()
{
	// 各3Dオブジェクトの更新
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


	// 毎フレーム、現在の透明度をパーティクルグループに反映
	// パーティクルの色（含むアルファ）を反映
	particle->SetColorToGroup(groupName, particleColor);
	particle->SetVelocityToGroup(groupName, particleVelocity);


	// 通常パーティクル更新
	particle->SetColorToGroup(groupName, particleColor);
	particle->SetVelocityToGroup(groupName, particleVelocity);

	/*planeParticle2->SetColorToGroup(groupName, { 0.0f, 0.0f, 0.0f, 1.0f });
	planeParticle2->SetVelocityToGroup(groupName, particleVelocity);*/


	// 敵が死んだら一度だけ爆発エフェクトを開始
	if (enemy_->IsDead() && !hasExploded_) {
		hasExploded_ = true;
		explosionEffectStart = true;

		// 爆発の位置を敵の位置にセット
		emitterTransform.translate = enemy_->GetTransform().translate;  // EnemyにGetTranslate()が必要
	}


	// 爆破エフェクト
	Explosion();


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

	// パーティクル描画
	planeParticle2->Draw();
	particle->Draw();


	// パーティクル描画（リング爆発）
	ringParticle->Draw();

	primitiveParticle->Draw();

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

void GamePlayScene::Explosion()
{
	if (explosionEffectStart == true)
	{
		// 全体の時間を記録（毎フレーム加算）
		static float totalTime = 0.0f;
		totalTime += 0.1f; //

		if (totalTime >= 0.6f)
		{
			static float emitterScale = 0.0f;
			emitterScale += 0.1f;
			if (emitterScale > 1.0f) emitterScale = 1.0f;
			emitterTransform.scale = { emitterScale, emitterScale, emitterScale };
			particle->SetScaleToGroup(groupName, emitterTransform.scale);

			if (emitter) {
				emitter->SetPosition(emitterTransform.translate);
				emitter->SetRotation(emitterTransform.rotate);
				emitter->SetScale(emitterTransform.scale);
				emitter->Update();
			}
			particle->Update();
		}

		if (totalTime >= 0.8f)
		{
			if (emitter2) {
				emitter2->SetPosition(emitterTransform.translate);
				emitter2->SetRotation(emitterTransform.rotate);
				emitter2->SetScale(emitterTransform.scale);
				emitter2->Update();
			}
			planeParticle2->SetColorToGroup(groupName2, { 0.0f, 0.0f, 0.0f, 0.2f });
			planeParticle2->SetVelocityToGroup(groupName2, particleVelocity);
			planeParticle2->SetScaleToGroup(groupName2, emitterTransform.scale);
			planeParticle2->Update();
		}


		// ===== リングスケールと透明度を時間で変化させ再Emit =====

		if (totalTime >= 0.3f) {

			static float ringScale = 0.0f;
			static float ringAlpha = 1.0f;

			ringScale += 0.5f;
			ringAlpha -= 0.02f;
			if (ringAlpha < 0.0f) ringAlpha = 0.0f;

			ringParticle->SetScaleToGroup("ring_billboard", { ringScale, ringScale, ringScale });
			ringParticle->SetScaleToGroup("ring_static", { ringScale, ringScale, ringScale });
			ringParticle->SetColorToGroup("ring_billboard", { 1.0f, 1.0f, 1.0f, ringAlpha });
			ringParticle->SetColorToGroup("ring_static", { 1.0f, 1.0f, 1.0f, ringAlpha });

			for (auto& ring : ringExplosions) {
				if (ring) {
					ring->Update();
				}
			}
			ringParticle->Update();
		}



		// ===== 放射線状のエフェクト（Primitive）をUpdateで閃光風に伸ばす =====
		// ===== Primitive パーティクルの毎フレームEmit + 更新（動的スケール＆回転） =====

		static float time = 0.0f;
		time += 0.1f;

		for (int i = 0; i < primitiveEmitters.size(); ++i) {
			float emitDelay = (static_cast<float>(i) / 2.0f) * 0.2f;

			if (totalTime >= emitDelay) {
				float scaleY = 4.5f + 4.5f * std::sin(time + i);
				float rotationZ = 3.1415f * 0.25f * std::sin(time + i * 0.5f);

				Transform t;
				t.translate = { 0.0f, 0.0f, 0.0f };
				t.scale = { 0.1f, scaleY, 1.0f };
				t.rotate = {
					0.0f,
					3.1415f * 2.0f * i / static_cast<float>(primitiveEmitters.size()),
					rotationZ
				};

				primitiveEmitters[i]->SetTransform(t);
				primitiveEmitters[i]->Update();
			}
		}
		primitiveParticle->Update();
	}
}


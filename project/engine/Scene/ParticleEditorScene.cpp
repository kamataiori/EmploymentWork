#include "ParticleEditorScene.h"
#include <GlobalVariables.h>
#include <Input.h>

void ParticleEditorScene::Initialize()
{
	// ===== ライト初期化 =====
	BaseScene::GetLight()->Initialize();
	BaseScene::GetLight()->GetCameraLight();
	BaseScene::GetLight()->GetDirectionalLight();
	BaseScene::GetLight()->SetDirectionalLightIntensity({ 1.0f });
	BaseScene::GetLight()->SetDirectionalLightColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// ===== カメラ初期化 =====
	camera = std::make_unique<Camera>();
	camera->SetTranslate({ 0.0f, 0.0f, -20.0f });  // 少し離した位置に設定
	camera->SetRotate({ 0.0f, 0.0f, 0.0f });

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(camera.get());

	// ===== パーティクル初期化 =====
	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->CreateParticleGroup(groupName, "Resources/smoke.png", ParticleManager::BlendMode::kBlendModeAdd);
	particle->SetCamera(camera.get());

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
	planeParticle2->SetCamera(camera.get());

	emitter2 = std::make_unique<ParticleEmitter>();
	emitter2->Initialize(planeParticle2.get(), groupName2, emitterTransform,
		EmitterConfig{ ShapeType::Plane, 20, 1.0f, false });
	emitter2->SetTransform(emitterTransform);
	emitter2->SetUseRandom(useRandomPosition);

	planeParticle2->SetColorToGroup(groupName2, { 0.0f, 0.0f, 0.0f, 0.2f });
	planeParticle2->SetVelocityToGroup(groupName2, particleVelocity);
	planeParticle2->SetLifeTimeToGroup(groupName2, 2.2f);
	planeParticle2->SetCurrentTimeToGroup(groupName2, particleStartTime);



	// ===== UI配置 =====
	AddRightDockWindow(kWindowName_Particle);


	// ===== リング用パーティクルマネージャ初期化 =====
	ringParticle->Initialize(ParticleManager::VertexDataType::Ring);
	ringParticle->SetCamera(camera.get());

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
	primitiveParticle->SetCamera(camera.get());

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


void ParticleEditorScene::Finalize()
{
}

void ParticleEditorScene::Update()
{
	// カメラ更新
	if (camera) camera->Update();

	skybox->Update();

	// 毎フレーム、現在の透明度をパーティクルグループに反映
	// パーティクルの色（含むアルファ）を反映
	particle->SetColorToGroup(groupName, particleColor);
	particle->SetVelocityToGroup(groupName, particleVelocity);


	// 通常パーティクル更新
	particle->SetColorToGroup(groupName, particleColor);
	particle->SetVelocityToGroup(groupName, particleVelocity);

	/*planeParticle2->SetColorToGroup(groupName, { 0.0f, 0.0f, 0.0f, 1.0f });
	planeParticle2->SetVelocityToGroup(groupName, particleVelocity);*/


	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		explosionEffectStart = true;
	}

	// 爆破エフェクト
	Explosion();

	// デバッグ
	Debug();

	//emitterTransform.translate = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "position");
	//emitterTransform.rotate = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "rotation");
	//emitterTransform.scale = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "scale");
	//particleVelocity = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "velocity");
	////particleColor = GlobalVariables::GetInstance()->GetValue<Vector4>("Particle", "color");
	//particleLifeTime = GlobalVariables::GetInstance()->GetValue<float>("Particle", "lifetime");
	//particleStartTime = GlobalVariables::GetInstance()->GetValue<float>("Particle", "startTime");
	//config.count = GlobalVariables::GetInstance()->GetValue<int32_t>("Particle", "count");
	//config.frequency = GlobalVariables::GetInstance()->GetValue<float>("Particle", "frequency");
	//config.repeat = GlobalVariables::GetInstance()->GetValue<bool>("Particle", "repeat");
	//useRandomPosition = GlobalVariables::GetInstance()->GetValue<bool>("Particle", "useRandom");


}

void ParticleEditorScene::BackGroundDraw()
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

void ParticleEditorScene::Draw()
{

	skybox->Draw();

	// ================================================
	// ここからSkyBoxの描画
	// ================================================



	// ================================================
	// ここまでSkyBox個々の描画
	// ================================================


	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================



	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================



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

void ParticleEditorScene::ForeGroundDraw()
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

void ParticleEditorScene::Debug()
{
#ifdef _DEBUG
	if (!IsDockedImGuiEnabled()) return;

	ImGui::Begin(kWindowName_Particle);


	// --- Transform 編集 ---
	ImGui::Text("エミッターの位置・回転・拡大率を調整");
	ImGui::DragFloat3("位置(Position)", &emitterTransform.translate.x, 0.1f);
	ImGui::DragFloat3("回転(Rotation)", &emitterTransform.rotate.x, 0.1f);
	ImGui::DragFloat3("スケール(Scale)", &emitterTransform.scale.x, 0.1f);

	// --- ランダム発生制御 ---
	ImGui::Separator();
	ImGui::Text("発生位置の設定");
	ImGui::Checkbox("ランダムな位置に発生", &useRandomPosition);
	ImGui::Text("オフにするとエミッター位置から固定で発生");

	// --- 速度ベクトル調整 ---
	ImGui::Separator();
	ImGui::Text("パーティクルの速度（発生時に反映）");
	ImGui::DragFloat3("速度(Velocity)", &particleVelocity.x, 0.05f, -10.0f, 10.0f);

	// --- テクスチャ切り替え ---
	ImGui::Separator();
	ImGui::Text("パーティクルに使用するテクスチャ");

	if (ImGui::BeginCombo("テクスチャ(Texture)", textureFiles[selectedTextureIndex].c_str())) {
		for (int i = 0; i < textureFiles.size(); ++i) {
			bool isSelected = (selectedTextureIndex == i);
			if (ImGui::Selectable(textureFiles[i].c_str(), isSelected)) {
				selectedTextureIndex = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}


	// --- 色（RGBA）調整 ---
	ImGui::Separator();
	ImGui::Text("パーティクルの色（RGBA）を調整します");

	ImGui::ColorEdit4("色 (Color RGBA)", &particleColor.x);

	// --- パーティクルの寿命と初期時間設定 ---
	ImGui::Separator();
	ImGui::Text("パーティクルの寿命と開始時間を調整");

	// 寿命（lifeTime）
	ImGui::DragFloat("寿命 (LifeTime)", &particleLifeTime, 0.1f, 0.1f, 100.0f);
	ImGui::Text("パーティクルが消えるまでの時間（秒）");

	// 開始時間（currentTime）
	ImGui::DragFloat("開始時間 (CurrentTime)", &particleStartTime, 0.1f, 0.0f, particleLifeTime);
	ImGui::Text("生成時の経過時間。0.0から始まるのが通常");



	// --- パラメータ編集 ---
	ImGui::Separator();
	ImGui::Text("パーティクルの生成設定を調整");

	ImGui::DragInt("発生数(Count)", (int*)&config.count, 1, 1, 10000);
	ImGui::Text("1回の発生で何個パーティクルを出すか");

	ImGui::DragFloat("発生間隔(Frequency)", &config.frequency, 0.01f, 0.01f, 10.0f);
	ImGui::Text("何秒ごとに発生するか（秒）");

	ImGui::Checkbox("繰り返し(Repeat)", &config.repeat);
	ImGui::Text("オンにすると一定間隔で繰り返し発生");

	// --- 形状タイプ選択 ---
	ImGui::Separator();
	ImGui::Text("パーティクルの形状タイプ");

	const char* shapeItems[] = { "Plane（通常）", "Primitive（線状）", "Ring（リング状）", "Cylinder（円柱状）" };
	int shapeIndex = static_cast<int>(config.shapeType);
	if (ImGui::Combo("形状タイプ", &shapeIndex, shapeItems, IM_ARRAYSIZE(shapeItems))) {
		config.shapeType = static_cast<ShapeType>(shapeIndex);
	}

	ImGui::Checkbox("カメラ目線 (ビルボード)", &useBillboard);
	particle->SetUseBillboard(useBillboard);


	// --- ブレンドモード選択 ---
	ImGui::Separator();
	ImGui::Text("描画時のブレンド方法");

	const char* blendItems[] = {
		"None（ブレンドなし）",      // kBlendModeNone
		"Normal（通常α）",          // kBlendModeNormal
		"Add（加算）",              // kBlendModeAdd
		"Subtract（減算）",         // kBlendModeSubtract
		"Multiply（乗算）",         // kBlendModeMultiply
		"Screen（スクリーン）"      // kBlendModeScreen
	};
	ImGui::Combo("ブレンドモード(Blend Mode)", &blendModeIndex, blendItems, IM_ARRAYSIZE(blendItems));

	// --- 再初期化ボタン ---
	ImGui::Separator();
	ImGui::Text("設定を反映してエミッターを再作成");

	if (ImGui::Button("エミッター再初期化(Re-Initialize)")) {
		// 既存のパーティクルグループを削除してから再作成（重要）
		particle->DeleteParticleGroup(groupName);

		// 新しいテクスチャでグループ再作成
		particle->CreateParticleGroup(
			groupName,
			textureFiles[selectedTextureIndex],
			static_cast<ParticleManager::BlendMode>(blendModeIndex)
		);

		// パラメータを再適用
		particle->SetColorToGroup(groupName, particleColor);
		particle->SetVelocityToGroup(groupName, particleVelocity);
		particle->SetLifeTimeToGroup(groupName, particleLifeTime);
		particle->SetCurrentTimeToGroup(groupName, particleStartTime);

		emitter->Initialize(particle.get(), groupName, emitterTransform, config);
		emitter->SetTransform(emitterTransform);
		emitter->SetUseRandom(useRandomPosition);  // ランダム設定反映
	}

	//emitterTransform.translate = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "position");
	//emitterTransform.rotate = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "rotation");
	//emitterTransform.scale = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "scale");
	//particleVelocity = GlobalVariables::GetInstance()->GetValue<Vector3>("Particle", "velocity");
	////particleColor = GlobalVariables::GetInstance()->GetValue<Vector4>("Particle", "color");
	//particleLifeTime = GlobalVariables::GetInstance()->GetValue<float>("Particle", "lifetime");
	//particleStartTime = GlobalVariables::GetInstance()->GetValue<float>("Particle", "startTime");
	//config.count = GlobalVariables::GetInstance()->GetValue<int32_t>("Particle", "count");
	//config.frequency = GlobalVariables::GetInstance()->GetValue<float>("Particle", "frequency");
	//config.repeat = GlobalVariables::GetInstance()->GetValue<bool>("Particle", "repeat");
	//useRandomPosition = GlobalVariables::GetInstance()->GetValue<bool>("Particle", "useRandom");


	ImGui::End();
#endif
}

void ParticleEditorScene::Explosion()
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

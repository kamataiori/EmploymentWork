#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <PostEffectManager.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <memory>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <algorithm>


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
	plane = std::make_unique<Object3d>(this);
	plane->Initialize();

	animationCube = std::make_unique<Object3d>(this);
	animationCube->Initialize();

	sneak = std::make_unique<Object3d>(this);
	sneak->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");
	ModelManager::GetInstance()->LoadModel("human/walk.gltf");
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("test.obj");
	plane->SetModel("uvChecker.gltf");
	animationCube->SetModel("human/walk.gltf");
	sneak->SetModel("human/sneakWalk.gltf");

	// モデルにSRTを設定
	plane->SetScale({ 1.0f, 1.0f, 1.0f });
	plane->SetRotate({ 0.0f, 3.14f, 0.0f });
	plane->SetTranslate({ 0.0f, 0.0f, 6.0f });

	sneak->SetTranslate({ 1.0f,0.0f,0.0f });

	// 3Dカメラの初期化
	camera1 = std::make_unique<Camera>();
	camera1->SetTranslate({ 0.0f, 0.0f, -15.0f });
	camera1->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera1->SetFarClip(2000.0f);

	// カメラのセット
	plane->SetCamera(camera1.get());
	particle->SetCamera(camera1.get());
	animationCube->SetCamera(camera1.get());
	sneak->SetCamera(camera1.get());
	primitiveParticle->SetCamera(camera1.get());
	ringParticle->SetCamera(camera1.get());
	cyrinderParticle->SetCamera(camera1.get());

	// ---- 各パーティクルマネージャの初期化とグループ作成 ----

// Plane
	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->CreateParticleGroup("particle", "Resources/circle.png", ParticleManager::BlendMode::kBlendModeAdd);
	auto emitter = std::make_unique<ParticleEmitter>();
	emitter->Initialize(
		particle.get(),
		"particle",
		Transform{ {1.0f, 1.0f, -4.0f}, {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f} },
		EmitterConfig{ ShapeType::Plane, 10, 0.5f, true }
	);
	emitters.push_back(std::move(emitter));

	// Primitive
	primitiveParticle->Initialize(ParticleManager::VertexDataType::Plane);
	primitiveParticle->CreateParticleGroup("primitive", "Resources/circle2.png");
	auto primitiveEmitter = std::make_unique<ParticleEmitter>();
	primitiveEmitter->Initialize(
		primitiveParticle.get(),
		"primitive",
		Transform{ {1.0f, 1.0f, -4.0f}, {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f} },
		EmitterConfig{ ShapeType::Primitive, 8, 0.5f, true }
	);
	primitiveEmitters.push_back(std::move(primitiveEmitter));

	// Ring
	ringParticle->Initialize(ParticleManager::VertexDataType::Ring);
	ringParticle->CreateParticleGroup("ring", "Resources/gradationLine.png");
	auto ringEmitter = std::make_unique<ParticleEmitter>();
	ringEmitter->Initialize(
		ringParticle.get(),
		"ring",
		Transform{ {1.0f, 1.0f, 1.0f}, {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f} },
		EmitterConfig{ ShapeType::Ring }
	);
	ringEmitters.push_back(std::move(ringEmitter));

	// Cylinder
	cyrinderParticle->Initialize(ParticleManager::VertexDataType::Cylinder);
	cyrinderParticle->CreateParticleGroup("cyrinder", "Resources/gradationLine.png");
	auto cyrinderEmitter = std::make_unique<ParticleEmitter>();
	cyrinderEmitter->Initialize(
		cyrinderParticle.get(),
		"cyrinder",
		Transform{ {1.0f, 1.0f, 1.0f}, {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f} },
		EmitterConfig{ ShapeType::Cylinder }
	);
	cyrinderEmitters.push_back(std::move(cyrinderEmitter));
	cyrinderParticle->SetFlipYToGroup("cyrinder", true);


	DrawLine::GetInstance()->SetCamera(camera1.get());
	aabb.min = { -1.8f, 2.2f, 3.0f }; // AABB の最小点を少し下げる
	aabb.max = { 1.8f, 1.1f, 0.5f };  // AABB の最大点を少し上げる
	aabb.color = static_cast<int>(Color::WHITE); // AABBの色を赤に設定
	sphere = { {-4.0f, -1.2f, 0.0f}, 1.0f, static_cast<int>(Color::WHITE) };
	ground.normal = { 0.0f, 1.0f, 0.0f }; // Y軸方向を法線とする平面
	ground.distance = -2.0f;             // 原点を通る平面
	ground.size = 6.0f;        // 平面のサイズ
	ground.divisions = 10;     // グリッドの分割数
	// カプセルの初期値
	capsule.start = { 1.6f, 0.0f, 0.0f };
	capsule.end = { 1.6f, -1.5f, 0.0f };
	capsule.radius = 0.5f;
	capsule.color = static_cast<int>(Color::WHITE);
	capsule.segments = 16; // 円周を構成する分割数
	capsule.rings = 8;     // 球部分を構成する分割数
	// OBB の初期化
	obb.center = { -1.9f, -0.3f, 0.0f };
	obb.orientations[0] = { 1.0f, 0.0f, 0.0f }; // X軸
	obb.orientations[1] = { 0.0f, 1.0f, 0.0f }; // Y軸
	obb.orientations[2] = { 0.0f, 0.0f, 1.0f }; // Z軸
	obb.size = { 1.0f, 1.0f, 0.5f }; // 各軸方向の半サイズ
	obb.color = static_cast<int>(Color::WHITE); // 色の初期値

	// DrawTriangleの初期化
	drawTriangle_ = DrawTriangle::GetInstance();
	//drawTriangle_->Initialize();
	drawTriangle_->SetCamera(camera1.get());

	GlobalVariables::GetInstance()->AddValue<Vector3>("Camera", "position", camera1->GetTranslate());
	GlobalVariables::GetInstance()->AddValue<Vector3>("Camera", "rotate", camera1->GetRotate());

	GlobalVariables::GetInstance()->AddValue<Vector3>("Animation", "position", animationCube->GetTranslate());
	GlobalVariables::GetInstance()->AddValue<Vector3>("Animation", "rotate", animationCube->GetRotate());

	// ---- Dock配置登録（BaseSceneの機能） ----
	AddBottomDockWindow(kWindowName_ParticleControl);

	AddRightDockWindow(kWindowName_AABBControl);
	AddRightDockWindow(kWindowName_OBBControl);
	AddRightDockWindow(kWindowName_SphereControl);
	AddLeftDockWindow(kWindowName_DebugInfo);

	fade_ = std::make_unique<Fade>();
	fade_->Initialize();

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(camera1.get());

	sceneController_ = std::make_unique<SceneController>(this);
	sceneController_->LoadScene("test"); // Resources/Json/test.json を読み込む
	sceneController_->SetCamera(camera1.get());

	title = std::make_unique<Sprite>();
	title->Initialize("Resources/title.png");

	// 画面サイズは 1280x720 固定
	const float screenW = 1280.0f;
	const float screenH = 720.0f;

	// -------------------------------
	// タイトル（タ・イ・ト・ル）
	// -------------------------------
	const float titleY = 160.0f; // 最終ライン
	float gap = 28.0f;          // 文字間（小さくすると詰まる）
	const float widthScaleTitle = 0.2f; // 実効幅係数（小さいほど詰まる）

	// テクスチャ（実ファイル名に合わせて変更OK）
	const char* kTitleTex[4] = {
		"Resources/numberT.png",  // タ（ここは仮名→実 PNG に合わせて）
		"Resources/numberI.png",  // イ
		"Resources/numberTo.png", // ト
		"Resources/numberR.png"   // ル
	};

	// 一旦読み込んでサイズ取得
	std::vector<std::unique_ptr<Sprite>> temp;
	for (int i = 0; i < 4; i++) {
		auto sp = std::make_unique<Sprite>();
		sp->Initialize(kTitleTex[i]);
		sp->SetAnchorPoint({ 0.5f, 0.5f });
		temp.push_back(std::move(sp));
	}

	// 総幅（実効幅）でセンタリング
	float totalW = 0.0f;
	for (auto& s : temp) totalW += s->GetSize().x * widthScaleTitle;
	totalW += gap * 3.0f; // 4文字→隙間3つ

	float cursor = (screenW - totalW) * 0.5f;
	for (int i = 0; i < 4; i++) {
		LetterAnim L;
		L.sp = std::move(temp[i]);

		float wEff = L.sp->GetSize().x * widthScaleTitle;
		float x = cursor + wEff * 0.5f; // アンカー(0.5)なので中央合わせ
		L.goal = { x, titleY };
		L.start = { x, -L.sp->GetSize().y - 40.0f }; // 画面上から
		L.delay = i * 0.20f;  // 左→右へ 0.20 秒刻み
		L.duration = 0.75f;   // ゆっくり目
		titleLetters_.push_back(std::move(L));

		cursor += wEff + gap;
	}

	// -------------------------------
	// space（s p a c e）
	// -------------------------------
	const float spaceY = 520.0f; // 最終ライン
	float gap2 = 20.0f;          // 文字間
	const float widthScaleSpace = 0.20f; // 実効幅係数

	const char* kSpaceTex[5] = {
		"Resources/s.png",
		"Resources/p.png",
		"Resources/a.png",
		"Resources/c.png",
		"Resources/e.png"
	};

	std::vector<std::unique_ptr<Sprite>> temp2;
	for (int i = 0; i < 5; i++) {
		auto sp = std::make_unique<Sprite>();
		sp->Initialize(kSpaceTex[i]);
		sp->SetAnchorPoint({ 0.5f, 0.5f });
		temp2.push_back(std::move(sp));
	}

	float totalW2 = 0.0f;
	for (auto& s : temp2) totalW2 += s->GetSize().x * widthScaleSpace;
	totalW2 += gap2 * 4.0f;  // 5文字→隙間4つ

	float cursor2 = (screenW - totalW2) * 0.5f;
	const int order[5] = { 4, 3, 2, 1, 0 }; // e→c→a→p→s の順で出す

	for (int i = 0; i < 5; i++) {
		LetterAnim L;
		L.sp = std::move(temp2[i]);

		float wEff = L.sp->GetSize().x * widthScaleSpace;
		float x = cursor2 + wEff * 0.5f;
		L.goal = { x, spaceY };
		L.start = { x, screenH + L.sp->GetSize().y + 40.0f }; // 画面下から

		int idx = int(std::find(std::begin(order), std::end(order), i) - std::begin(order));
		L.delay = idx * 0.20f;
		L.duration = 0.75f;
		spaceLetters_.push_back(std::move(L));

		cursor2 += wEff + gap2;
	}



	ModelManager::GetInstance()->LoadModel("skydome.obj");
	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });
	sky->SetCamera(camera1.get());


	// ===== シャッター用 Black.png を2枚作る =====
	// 画面サイズ（固定ならその値、可変ならシステムから取得してください）
	/*const float screenW = 1280.0f;
	const float screenH = 720.0f;*/

	shutterTop_ = std::make_unique<Sprite>();
	shutterBottom_ = std::make_unique<Sprite>();

	shutterTop_->Initialize("Resources/Black.png");
	shutterBottom_->Initialize("Resources/Black.png");

	shutterTop_->SetAnchorPoint({ 0.5f, 1.0f });
	shutterBottom_->SetAnchorPoint({ 0.5f, 0.0f });

	shutterTop_->SetSize({ screenW, screenH * 0.55f });
	shutterBottom_->SetSize({ screenW, screenH * 0.55f });

	shutter_.topStart = { screenW * 0.5f, 0.0f };
	shutter_.botStart = { screenW * 0.5f, screenH };
	shutter_.topEnd = { screenW * 0.5f, screenH * 0.5f };
	shutter_.botEnd = { screenW * 0.5f, screenH * 0.5f };

	shutterTop_->SetPosition(shutter_.topStart);
	shutterBottom_->SetPosition(shutter_.botStart);
}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{
	title->Update();
	sky->Update();
	//// アルファ値を減少させる
	//Vector4 color = plane->GetMaterialColor();
	////color.w = 0.5f;
	//color.w -= 0.01f; // アルファ値を減少
	//if (color.w < 0.0f) {
	//	color.w = 0.0f; // 最小値を0に制限
	//}
	//plane->SetMaterialColor(color);

	const float dt = 1.0f / 60.0f;

	// === SPACE押下でシャッター演出開始 ===
	if (!shutter_.active && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		BeginShutterExit("GAMEPLAY", 1.2f /*ゆっくり*/, 0.5f /*真っ黒ホールド*/);
	}

	// === シャッター進行 ===
	if (shutter_.active) {
		if (shutter_.t < 1.0f) {
			shutter_.t = std::min(1.0f, shutter_.t + dt / (std::max)(0.001f, shutter_.duration));
			float e = EaseInOutCubic(shutter_.t);

			// 上下を補間
			Vector2 topPos = LerpVec2(shutter_.topStart, shutter_.topEnd, e);
			Vector2 botPos = LerpVec2(shutter_.botStart, shutter_.botEnd, e);

			shutterTop_->SetPosition(topPos);
			shutterBottom_->SetPosition(botPos);
		}
		else {
			// 完全に閉じた状態（真っ黒）
			shutterTop_->SetPosition(shutter_.topEnd);
			shutterBottom_->SetPosition(shutter_.botEnd);

			// 閉じ切り後、ホールド時間を計測
			shutter_.holdTimer += dt;
			if (shutter_.holdTimer >= shutter_.holdSec) {
				SceneManager::GetInstance()->ChangeScene(shutter_.nextScene);
				shutter_ = {}; // リセット
			}
		}

		shutterTop_->Update();
		shutterBottom_->Update();
	}

	// 各3Dオブジェクトの更新
	plane->Update();

	/*animationCube->SetTranslate(GlobalVariables::GetInstance()->GetValue<Vector3>("Animation", "position"));
	animationCube->SetRotate(GlobalVariables::GetInstance()->GetValue<Vector3>("Animation", "rotate"));
	camera1->SetTranslate(GlobalVariables::GetInstance()->GetValue<Vector3>("Camera", "position"));
	camera1->SetRotate(GlobalVariables::GetInstance()->GetValue<Vector3>("Camera", "rotate"));*/

	animationCube->Update();
	sneak->Update();
	// カメラの更新
	camera1->Update();

	//const float dt = 1.0f / 60.0f;
	animClock_ += dt;

	// タイトル各文字
	for (auto& L : titleLetters_) {
		if (animClock_ >= L.delay) {
			L.t = std::min(1.0f, L.t + dt / L.duration);
			float e = EaseOutBack(L.t);
			Vector2 pos = LerpVec2(L.start, L.goal, e);
			L.sp->SetPosition(pos);
		}
		else {
			L.sp->SetPosition(L.start);
		}
		L.sp->Update();
	}

	// space 各文字
	for (auto& L : spaceLetters_) {
		if (animClock_ >= L.delay) {
			L.t = std::min(1.0f, L.t + dt / L.duration);
			float e = EaseOutBack(L.t);
			Vector2 pos = LerpVec2(L.start, L.goal, e);
			L.sp->SetPosition(pos);
		}
		else {
			L.sp->SetPosition(L.start);
		}
		L.sp->Update();
	}


	for (auto& emitter : emitters)
	{
		emitter->Update();
	}

	if (changeSpeed_) {
		particle->SetVelocityToGroup("particle", { 0.0f, 1.5f, 0.0f }); // 速い速度
	}
	else {
		particle->SetVelocityToGroup("particle", { 0.0f, 0.1f, 0.0f }); // 遅い速度
	}
	for (auto& PrimitiveEmitter : primitiveEmitters)
	{
		PrimitiveEmitter->Update();
	}

	for (auto& ringEmitter : ringEmitters)
	{
		ringEmitter->Update();
	}

	for (auto& cyrinderEmitter : cyrinderEmitters)
	{
		cyrinderEmitter->Update();
	}

	particle->Update();
	primitiveParticle->Update();
	ringParticle->Update();
	cyrinderParticle->Update();

	//PostEffectManager::GetInstance()->SetType(PostEffectType::Sepia);

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}
	if (Input::GetInstance()->TriggerKey(DIK_I)) {
		PostEffectManager::GetInstance()->SetGrayscaleWeights({ 0.299f, 0.587f, 0.114f });
	}
	if (Input::GetInstance()->TriggerKey(DIK_L)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
	}
	if (Input::GetInstance()->TriggerKey(DIK_O)) {
		PostEffectManager::GetInstance()->SetVignetteColor({ 1.0f,0.85f,0.3f });
		PostEffectManager::GetInstance()->SetVignettePower(1.0f);
		PostEffectManager::GetInstance()->SetVignetteScale(1.0f);
	}
	if (Input::GetInstance()->TriggerKey(DIK_J)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Sepia);
	}
	if (Input::GetInstance()->TriggerKey(DIK_M)) {
		PostEffectManager::GetInstance()->SetSepiaColor({ 0.4f, 0.3f, 0.9f });
		PostEffectManager::GetInstance()->SetSepiaStrength(0.9f);
	}
	if (Input::GetInstance()->TriggerKey(DIK_B)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::RadialBlur);
	}
	if (Input::GetInstance()->TriggerKey(DIK_G)) {
		//PostEffectManager::GetInstance()->SetRadialBlurCenter({ 0.2f,0.2f });
		PostEffectManager::GetInstance()->SetRadialBlurWidth(0.05f);
	}
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Random);
		//PostEffectManager::GetInstance()->Set
	}
	if (Input::GetInstance()->TriggerKey(DIK_E)) {
		// 元画像にちらつくノイズを乗せる
		PostEffectManager::GetInstance()->SetRandomUseImage(true);

	}

	if (Input::GetInstance()->TriggerKey(DIK_D)) {
		isDissolve = true;
		PostEffectManager::GetInstance()->SetType(PostEffectType::Dissolve);
		PostEffectManager::GetInstance()->DissolveInitialize(0.3f, 0.03f, { 1.0f, 0.4f, 0.3f });
		// 使用するテクスチャを指定
		PostEffectManager::GetInstance()->SetDissolveTextures(
			"Resources/noise1.png",     // 通常シーンの画像（gTexture）
			"Resources/noise0.png"            // マスク画像（gMaskTexture）
		);

		// しきい値を ImGuiなどでリアルタイム制御も可能
		PostEffectManager::GetInstance()->SetDissolveThreshold(sliderValue);
	}

	if (isDissolve = true)
	{
		//ImGui::Begin("PostEffect Controller");

		//// Dissolve 関連 UI

		//static float threshold = 0.4f;
		//static float edgeWidth = 0.03f;
		//static Vector3 edgeColor = { 1.0f, 0.4f, 0.3f };

		//ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f);
		//ImGui::SliderFloat("Edge Width", &edgeWidth, 0.0f, 0.1f);
		//ImGui::ColorEdit3("Edge Color", &edgeColor.x);

		//PostEffectManager::GetInstance()->SetDissolveThreshold(threshold);
		//PostEffectManager::GetInstance()->SetDissolveEdgeWidth(edgeWidth);
		//PostEffectManager::GetInstance()->SetDissolveEdgeColor(edgeColor);

		//ImGui::End();
	}


	skybox->Update();

	// デバッグ
	//Debug();

	sceneController_->Update();


	//if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
	//	// シーン切り替え
	//	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	//}

	//if (Input::GetInstance()->TriggerKey(DIK_U)) {
	//	// シーン切り替え
	//	SceneManager::GetInstance()->ChangeScene("Unity");
	//}

	// フェード処理
	//if (fade_) {
	//	fade_->Update();

	//	// フェードアウト完了後にシーン遷移
	//	if (!nextSceneName_.empty() && !fade_->IsFinish()) {
	//		SceneManager::GetInstance()->ChangeScene(nextSceneName_);
	//		nextSceneName_.clear(); // 一度きりでリセット
	//	}
	//}

	//// キー入力でフェード開始（シーン遷移予約）
	//if (!fade_->IsActive()) {
	//	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
	//		PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
	//		fade_->Start(Fade::Status::FadeOut, 2.0f);
	//		nextSceneName_ = "GAMEPLAY";
	//	}
	//	if (Input::GetInstance()->TriggerKey(DIK_U)) {
	//		fade_->Start(Fade::Status::FadeOut, 2.0f);
	//		nextSceneName_ = "Unity";
	//	}
	//	if (Input::GetInstance()->TriggerKey(DIK_P)) {
	//		fade_->Start(Fade::Status::FadeOut, 2.0f);
	//		nextSceneName_ = "PARTICLE";
	//	}
	//}

	// --- Vignette Exit 進行 ---
	if (vignetteExit_.active) {
		// 目標値へ近づける
		vignetteExit_.scale = std::min(vignetteExit_.targetScale,
			vignetteExit_.scale + vignetteExit_.speedScale * dt);
		vignetteExit_.power = std::min(vignetteExit_.targetPower,
			vignetteExit_.power + vignetteExit_.speedPower * dt);

		PostEffectManager::GetInstance()->SetVignetteColor(vignetteExit_.color);
		PostEffectManager::GetInstance()->SetVignetteScale(vignetteExit_.scale);
		PostEffectManager::GetInstance()->SetVignettePower(vignetteExit_.power);

		// 目標へ到達 → 真っ黒で少しホールド → 切替
		const bool reached =
			vignetteExit_.scale >= vignetteExit_.targetScale &&
			vignetteExit_.power >= vignetteExit_.targetPower;

		if (reached) {
			vignetteExit_.holdTimer += dt;
			if (vignetteExit_.holdTimer >= vignetteExit_.holdBlackSec) {
				SceneManager::GetInstance()->ChangeScene(vignetteExit_.nextScene);
				vignetteExit_ = {};
			}
		}
	}

	//if (Input::GetInstance()->TriggerKey(DIK_SPACE) && !vignetteExit_.active) {
	//	// より濃い終端（targetScale/targetPower を上げる）＋ゆっくり速度
	//	BeginVignetteExit("GAMEPLAY", { 0,0,0 },
	//		0.0f, 0.0f,
	//		0.3f,          // 速度（ゆっくり）
	//		1.30f, 2.6f);  // 終端の濃さ（強くしたいほど上げる）
	//}



	// --- Vignette 版シーン切替のトリガ ---
	/*if (!vignetteExit_.active) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			BeginVignetteExit("GAMEPLAY", { 0.0f,0.0f,0.0f }, 0.0f, 0.0f, 0.2f);
		}
		if (Input::GetInstance()->TriggerKey(DIK_U)) {
			BeginVignetteExit("Unity", { 0.0f,0.0f,0.0f }, 0.0f, 0.0f, 1.6f);
		}
		if (Input::GetInstance()->TriggerKey(DIK_P)) {
			BeginVignetteExit("PARTICLE", { 0.0f,0.0f,0.0f }, 0.0f, 0.0f, 1.6f);
		}
	}*/


	// TitleScene::Update() に強制起動テスト
	if (Input::GetInstance()->TriggerKey(DIK_F)) {
		fade_->Start(Fade::Status::FadeOut, 1.0f);
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
	skybox->Draw();

	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	//plane->Draw();

	//sceneController_->Draw();

	//sky->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	animationCube->Draw();
	sneak->Draw();

	// ================================================
	// ここまでアニメーションオブジェクトの個々の描画
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
	// DrawTriangleの描画
	// 初期三角形を追加
	//drawTriangle_->AddTriangle(triangleP1, triangleP2, triangleP3, triangleColor, triangleAlpha);

	//DrawLine::GetInstance()->DrawAABB(aabb);
	//DrawLine::GetInstance()->DrawSphere(sphere);
	//// 平面の描画
	//DrawLine::GetInstance()->DrawPlane(ground);
	//// カプセルの描画
	//DrawLine::GetInstance()->DrawCapsule(capsule);
	//// OBB を描画
	//DrawLine::GetInstance()->DrawOBB(obb);

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
	// ===== シャッターを最前面に描く =====
	/*if (shutter_.active)*/ {
		shutterTop_->Draw();
		shutterBottom_->Draw();
	}

	// タイトル文字列
	for (auto& L : titleLetters_) { L.sp->Draw(); }
	for (auto& L : spaceLetters_) { L.sp->Draw(); }



	/*if (fade_) {
		fade_->Draw();
	}*/


	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	/*particle->Draw();
	primitiveParticle->Draw();
	ringParticle->Draw();
	cyrinderParticle->Draw();*/

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void TitleScene::Debug()
{
#ifdef _DEBUG

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など Scene UI
	//BaseScene::ShowFPS();


	ImGui::Begin(kWindowName_ParticleControl);
	ImGui::Checkbox("Change Speed", &changeSpeed_);
	ImGui::End();

	ImGui::Begin(kWindowName_DebugInfo); // デバッグ情報用ウィンドウ
	ImGui::Text("Number of Lines: %zu", DrawLine::GetInstance()->GetLineCount());
	ImGui::End();

	// AABB の編集
	ImGui::Begin(kWindowName_AABBControl);
	ImGui::Text("Adjust AABB parameters:");
	ImGui::DragFloat3("Min", &aabb.min.x, 0.1f); // AABB の最小点を調整
	ImGui::DragFloat3("Max", &aabb.max.x, 0.1f); // AABB の最大点を調整
	ImGui::End();

	// OBB の調整
	ImGui::Begin(kWindowName_OBBControl);
	ImGui::DragFloat3("Center", &obb.center.x, 0.1f); // 中心点
	ImGui::DragFloat3("Size", &obb.size.x, 0.1f, 0.1f, 10.0f); // 各軸方向の半サイズ
	ImGui::DragFloat3("Orientation X", &obb.orientations[0].x, 0.1f); // X軸方向
	ImGui::DragFloat3("Orientation Y", &obb.orientations[1].x, 0.1f); // Y軸方向
	ImGui::DragFloat3("Orientation Z", &obb.orientations[2].x, 0.1f); // Z軸方向
	ImGui::End();

	// Sphere の編集
	ImGui::Begin(kWindowName_SphereControl);
	ImGui::Text("Adjust Sphere parameters:");
	ImGui::DragFloat3("Center", &sphere.center.x, 0.1f); // Sphere の中心点を調整
	ImGui::DragFloat("Radius", &sphere.radius, 0.1f, 0.1f, 100.0f); // Sphere の半径を調整
	ImGui::End();

	// Plane の調整
	//ImGui::Begin("Ground Control");
	//ImGui::Text("Adjust Plane parameters:");
	//ImGui::DragFloat3("Normal", &ground.normal.x, 0.1f); // 法線を調整
	//ImGui::DragFloat("Distance", &ground.distance, 0.1f); // 距離を調整
	//ImGui::DragFloat("Size", &ground.size, 0.1f, 1.0f, 20.0f); // サイズを調整
	//ImGui::DragInt("Divisions", &ground.divisions, 1, 1, 50); // グリッド分割数を調整
	//ImGui::End();

	// Capsule の編集
	//ImGui::Begin("Capsule Control");
	//ImGui::DragFloat3("Start", &capsule.start.x, 0.1f); // 開始点を調整
	//ImGui::DragFloat3("End", &capsule.end.x, 0.1f);     // 終了点を調整
	//ImGui::DragFloat("Radius", &capsule.radius, 0.1f, 0.1f, 10.0f); // 半径を調整
	//ImGui::DragInt("Segments", &capsule.segments, 1, 4, 64); // 円周分割数を調整
	//ImGui::DragInt("Rings", &capsule.rings, 1, 2, 32);       // 球部分分割数を調整
	//ImGui::End();

	// DrawTriangleの更新
	//drawTriangle_->Update();

	// ImGui ウィンドウ
	//ImGui::Begin("Triangle Control");

	//// 頂点座標の変更
	//ImGui::DragFloat3("Vertex 1", &triangleP1.x, 0.1f);
	//ImGui::DragFloat3("Vertex 2", &triangleP2.x, 0.1f);
	//ImGui::DragFloat3("Vertex 3", &triangleP3.x, 0.1f);

	//// 変更を適用
	//if (ImGui::Button("Apply Changes")) {
	//	drawTriangle_->ResetData(); // データをクリア
	//	drawTriangle_->AddTriangle(triangleP1, triangleP2, triangleP3, triangleColor, triangleAlpha);
	//}

	//ImGui::End();
#endif
}

void TitleScene::BeginVignetteExit(const std::string& next,
	const Vector3& color = { 0,0,0 },
	float startScale = 0.0f, float startPower = 0.0f,
	float speed = 0.4f,
	float targetScale = 1.25f, float targetPower = 2.2f)
{
	vignetteExit_.active = true;
	vignetteExit_.nextScene = next;
	vignetteExit_.color = color;

	vignetteExit_.scale = startScale;
	vignetteExit_.power = startPower;
	vignetteExit_.speedScale = speed;
	vignetteExit_.speedPower = speed;

	vignetteExit_.targetScale = targetScale;
	vignetteExit_.targetPower = targetPower;

	vignetteExit_.holdTimer = 0.0f;

	PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
	PostEffectManager::GetInstance()->SetVignetteColor(vignetteExit_.color);
	PostEffectManager::GetInstance()->SetVignetteScale(vignetteExit_.scale);
	PostEffectManager::GetInstance()->SetVignettePower(vignetteExit_.power);
}

void TitleScene::BeginShutterExit(const std::string& next, float duration, float hold)
{
	shutter_.active = true;
	shutter_.nextScene = next;
	shutter_.duration = (std::max)(0.03f, duration);
	shutter_.holdSec = (std::max)(0.0f, hold);
	shutter_.holdTimer = 0.0f;
	shutter_.t = 0.0f;

	shutterTop_->SetPosition(shutter_.topStart);
	shutterBottom_->SetPosition(shutter_.botStart);
}

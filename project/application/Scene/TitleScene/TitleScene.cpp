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

	// カメラのセット
	plane->SetCamera(camera1.get());
	particle->SetCamera(camera1.get());
	animationCube->SetCamera(camera1.get());
	sneak->SetCamera(camera1.get());
	primitiveParticle->SetCamera(camera1.get());
	ringParticle->SetCamera(camera1.get());
	cyrinderParticle->SetCamera(camera1.get());

	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->CreateParticleGroup("particle", "Resources/circle.png", ParticleManager::BlendMode::kBlendModeAdd);
	//particle->CreateParticleGroup("particle2", "Resources/circle.png", ParticleManager::BlendMode::kBlendModeAdd,{32.0f,32.0f});
	// ParticleEmitterの初期化
	auto emitter = std::make_unique<ParticleEmitter>(particle.get(), "particle", Transform{ {0.0f, 0.0f, -4.0f} }, 10, 0.5f, true);
	emitters.push_back(std::move(emitter));

	primitiveParticle->Initialize(ParticleManager::VertexDataType::Plane);
	primitiveParticle->CreateParticleGroup("primitive", "Resources/circle2.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto primitiveEmitter = std::make_unique<ParticleEmitter>(primitiveParticle.get(), "primitive", Transform{ {0.0f, 0.0f, -4.0f} }, 8, 0.5f, true,true);
	primitiveEmitters.push_back(std::move(primitiveEmitter));

	ringParticle->Initialize(ParticleManager::VertexDataType::Ring);
	ringParticle->CreateParticleGroup("ring", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto ringEmitter = std::make_unique<ParticleEmitter>(ringParticle.get(), "ring", Transform{ {0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });
	ringEmitters.push_back(std::move(ringEmitter));

	cyrinderParticle->Initialize(ParticleManager::VertexDataType::Cylinder);
	cyrinderParticle->CreateParticleGroup("cyrinder", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto cyrinderEmitter = std::make_unique<ParticleEmitter>(cyrinderParticle.get(), "cyrinder", Transform{ {0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} },true);
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

} 

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{

	//// アルファ値を減少させる
	//Vector4 color = plane->GetMaterialColor();
	////color.w = 0.5f;
	//color.w -= 0.01f; // アルファ値を減少
	//if (color.w < 0.0f) {
	//	color.w = 0.0f; // 最小値を0に制限
	//}
	//plane->SetMaterialColor(color);

	// 各3Dオブジェクトの更新
	plane->Update();

	animationCube->SetTranslate(GlobalVariables::GetInstance()->GetValue<Vector3>("Animation", "position"));
	animationCube->SetRotate(GlobalVariables::GetInstance()->GetValue<Vector3>("Animation", "rotate"));
	camera1->SetTranslate(GlobalVariables::GetInstance()->GetValue<Vector3>("Camera", "position"));
	camera1->SetRotate(GlobalVariables::GetInstance()->GetValue<Vector3>("Camera", "rotate"));

	animationCube->Update();
	sneak->Update();
	// カメラの更新
	camera1->Update();

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

	// デバッグ
	Debug();
	

	//if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
	//	// シーン切り替え
	//	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	//}

	//if (Input::GetInstance()->TriggerKey(DIK_U)) {
	//	// シーン切り替え
	//	SceneManager::GetInstance()->ChangeScene("Unity");
	//}

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
		if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			nextSceneName_ = "GAMEPLAY";
		}
		if (Input::GetInstance()->TriggerKey(DIK_U)) {
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			nextSceneName_ = "Unity";
		}
	}

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
	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	plane->Draw();

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

	DrawLine::GetInstance()->DrawAABB(aabb);
	DrawLine::GetInstance()->DrawSphere(sphere);
	// 平面の描画
	DrawLine::GetInstance()->DrawPlane(ground);
	// カプセルの描画
	DrawLine::GetInstance()->DrawCapsule(capsule);
	// OBB を描画
	DrawLine::GetInstance()->DrawOBB(obb);

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

	if (fade_) {
		fade_->Draw();
	}


	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	particle->Draw();
	primitiveParticle->Draw();
	ringParticle->Draw();
	cyrinderParticle->Draw();

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

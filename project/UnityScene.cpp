#include "UnityScene.h"
#include <Input.h>
#include "SceneManager.h"

void UnityScene::Initialize()
{
	// ライト
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


	offscreenRendering_ = std::make_unique<OffscreenRendering>();
	offscreenRendering_->Initialize();

	// ImGuiスタイルの設定
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // 背景（灰色）
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // 子ウィンドウ
	colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // 境界線
	colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // ボタン/入力枠
	colors[ImGuiCol_Header] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // 折りたたみ等
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	colors[ImGuiCol_Button] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);


	// 3Dカメラの初期化
	camera1 = std::make_unique<Camera>();
	camera1->SetTranslate({ 0.0f, 0.0f, -15.0f });
	camera1->SetRotate({ 0.0f, 0.0f, 0.0f });

	// カメラにセット
	primitiveParticle->SetCamera(camera1.get());
	ringParticle->SetCamera(camera1.get());
	cyrinderParticle->SetCamera(camera1.get());

	primitiveParticle->Initialize(ParticleManager::VertexDataType::Plane);
	primitiveParticle->CreateParticleGroup("primitive", "Resources/circle2.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto primitiveEmitter = std::make_unique<ParticleEmitter>(primitiveParticle.get(), "primitive", Transform{ {0.0f, 0.0f, -4.0f} }, 8, 0.5f, true, true);
	primitiveEmitters.push_back(std::move(primitiveEmitter));

	ringParticle->Initialize(ParticleManager::VertexDataType::Ring);
	ringParticle->CreateParticleGroup("ring", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto ringEmitter = std::make_unique<ParticleEmitter>(ringParticle.get(), "ring", Transform{ {0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });
	ringEmitters.push_back(std::move(ringEmitter));

	cyrinderParticle->Initialize(ParticleManager::VertexDataType::Cylinder);
	cyrinderParticle->CreateParticleGroup("cyrinder", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto cyrinderEmitter = std::make_unique<ParticleEmitter>(cyrinderParticle.get(), "cyrinder", Transform{ {0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, true);
	cyrinderEmitters.push_back(std::move(cyrinderEmitter));

	cyrinderParticle->SetFlipYToGroup("cyrinder", true);
}

void UnityScene::Finalize()
{
}

void UnityScene::Update()
{

	camera1->Update();

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

	primitiveParticle->Update();
	ringParticle->Update();
	cyrinderParticle->Update();

	

	if (Input::GetInstance()->TriggerKey(DIK_T)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	if (Input::GetInstance()->TriggerKey(DIK_G)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	}
}

void UnityScene::BackGroundDraw()
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

void UnityScene::Draw()
{
	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	//offscreenRendering_->Draw();

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

void UnityScene::ForeGroundDraw()
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

	primitiveParticle->Draw();
	ringParticle->Draw();
	cyrinderParticle->Draw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void UnityScene::Debug()
{
}


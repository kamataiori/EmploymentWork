#include "ParticleScene.h"
#include <Input.h>

void ParticleScene::Initialize()
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

	// 3Dカメラの初期化
	camera1 = std::make_unique<Camera>();
	camera1->SetTranslate({ 0.0f, 0.0f, -15.0f });
	camera1->SetRotate({ 0.0f, 0.0f, 0.0f });

	cyrinderParticle->Initialize(ParticleManager::VertexDataType::Cylinder);
	cyrinderParticle->CreateParticleGroup("cyrinder", "Resources/gradationLine.png", ParticleManager::BlendMode::kBlendModeAdd);
	// ParticleEmitterの初期化
	auto cyrinderEmitter = std::make_unique<ParticleEmitter>(cyrinderParticle.get(), "cyrinder", Transform{ {0.0f, 0.0f, 0.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, true);
	cyrinderEmitters.push_back(std::move(cyrinderEmitter));

	cyrinderParticle->SetFlipYToGroup("cyrinder", true);

	cyrinderParticle->SetCamera(camera1.get());

	
}

void ParticleScene::Finalize()
{
}

void ParticleScene::Update()
{
	// カメラの更新
	camera1->Update();

	cyrinderParticle->Update(false);

	if (ImGui::Begin("Cylinder Particle Debug")) {

		static Vector3 scale = { 1.0f, 1.0f, 1.0f };
		static Vector3 rotate = { 0.0f, 0.0f, 0.0f };
		static Vector3 translate = { 0.0f, 0.0f, 0.0f };
		static Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		static Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		ImGui::Text("Cyrinder Particle Controls");

		ImGui::DragFloat3("Scale", &scale.x, 0.01f);
		ImGui::DragFloat3("Rotate", &rotate.x, 0.01f);
		ImGui::DragFloat3("Translate", &translate.x, 0.01f);
		ImGui::DragFloat3("Velocity", &velocity.x, 0.01f);
		ImGui::ColorEdit4("Color", &color.x);

		cyrinderParticle->SetScaleToGroup("cyrinder", scale);
		cyrinderParticle->SetRotationToGroup("cyrinder", rotate);
		cyrinderParticle->SetTranslationToGroup("cyrinder", translate);
		cyrinderParticle->SetVelocityToGroup("cyrinder", velocity);
		cyrinderParticle->SetColorToGroup("cyrinder", color);

	}
	ImGui::End();

}

void ParticleScene::BackGroundDraw()
{
}

void ParticleScene::Draw()
{
}

void ParticleScene::ForeGroundDraw()
{
	// ================================================
	// ここからparticle個々の描画
	// ================================================

	cyrinderParticle->Draw();


	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void ParticleScene::Debug()
{
	if (ImGui::Begin("Cylinder Particle Debug")) {

		static Vector3 scale = { 1.0f, 1.0f, 1.0f };
		static Vector3 rotate = { 0.0f, 0.0f, 0.0f };
		static Vector3 translate = { 0.0f, 0.0f, 0.0f };
		static Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		static Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		ImGui::Text("Cyrinder Particle Controls");

		ImGui::DragFloat3("Scale", &scale.x, 0.01f);
		ImGui::DragFloat3("Rotate", &rotate.x, 0.01f);
		ImGui::DragFloat3("Translate", &translate.x, 0.01f);
		ImGui::DragFloat3("Velocity", &velocity.x, 0.01f);
		ImGui::ColorEdit4("Color", &color.x);

		if (ImGui::Button("Apply to Group")) {
			cyrinderParticle->SetScaleToGroup("cyrinder", scale);
			cyrinderParticle->SetRotationToGroup("cyrinder", rotate);
			cyrinderParticle->SetTranslationToGroup("cyrinder", translate);
			cyrinderParticle->SetVelocityToGroup("cyrinder", velocity);
			cyrinderParticle->SetColorToGroup("cyrinder", color);
		}

	}
	ImGui::End();
}


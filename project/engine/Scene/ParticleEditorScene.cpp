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
	particle = std::make_unique<ParticleManager>();
	// デフォルトはPlaneとして初期化（Ring/CylinderプリセットもEmitByPresetNameから使える想定）
	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->SetCamera(camera.get());

	// 起動時に一括プリセット読み込み（Resources/Particle/*.json）
	particle->LoadAllPresets();

	// エミッターのデフォルト値（原点、スケール1）
	emitterTransform.scale = { 1.0f, 1.0f, 1.0f };
	emitterTransform.rotate = { 0.0f, 0.0f, 0.0f };
	emitterTransform.translate = { 0.0f, 0.0f, 1.0f };

	emitPresetName = "NewParticle"; // プリセットエディタのデフォルト名と合わせておく想定

#ifdef USE_IMGUI
	// 右側ドックにウィンドウを出す（レイアウト用）
	AddRightDockWindow(kWindowName_Particle);
	AddRightDockWindow(kWindowName_Preset);
#endif
}


void ParticleEditorScene::Finalize()
{
}

void ParticleEditorScene::Update()
{
	// カメラ更新
	if (camera) camera->Update();

	skybox->Update();

	// パーティクル更新
	if (particle) {
		particle->EmitByPresetName(emitPresetName, emitterTransform);
		particle->Update();
	}

	// デバッグ
	Debug();

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
	if (particle) {
		particle->Draw();
	}

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void ParticleEditorScene::Debug()
{
#ifdef USE_IMGUI
	if (!IsDockedImGuiEnabled()) { return; }

	// ==========================
	// 1) Emit 用コントロールウィンドウ
	// ==========================
	if (ImGui::Begin(kWindowName_Particle)) {

		ImGui::Text("このシーンはパーティクルプリセットの編集＆プレビュー用です。");
		ImGui::Separator();

		// Emit Transform 編集
		ImGui::Text("Emit Transform");
		ImGui::DragFloat3("Position", &emitterTransform.translate.x, 0.1f);
		ImGui::DragFloat3("Rotation", &emitterTransform.rotate.x, 0.1f);
		ImGui::DragFloat3("Scale", &emitterTransform.scale.x, 0.1f, 0.0f, 100.0f);

		ImGui::Separator();

		// Emit に使うプリセット名
		ImGui::Text("Emit Preset Name");
		char buf[64];
		// 安全な strncpy_s を使用
		strncpy_s(buf, sizeof(buf), emitPresetName.c_str(), _TRUNCATE);
		if (ImGui::InputText("Preset Name", buf, sizeof(buf))) {
			emitPresetName = buf;
		}

		if (ImGui::Button("Emit Now")) {
			if (particle) {
				particle->EmitByPresetName(emitPresetName, emitterTransform);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload All Presets")) {
			if (particle) {
				particle->LoadAllPresets();
			}
		}

		ImGui::Text("SPACE キーでも現在のプリセットを一度だけ Emit します。");
	}
	ImGui::End();

	// ==========================
	// 2) ParticleManager 側のプリセット編集ウィンドウ
	//    （JSON保存もここから）
	// ==========================
	if (particle) {
		particle->DrawImGuiParticlePresetEditor();  // ImGui::Begin("Particle Preset Editor") を内部で呼ぶ
	}
#endif
}

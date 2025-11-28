#include "ParticleEditorScene.h"
#include <GlobalVariables.h>
#include <Input.h>
#include "SceneManager.h"

#include <SrvManager.h>
#include "PostEffectManager.h"

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

	// ===== グリッド初期化 =====
	DrawLine::GetInstance()->SetCamera(camera.get());
	ground.normal = { 0.0f, 5.0f, 0.0f }; // Y軸方向を法線とする平面
	ground.distance = -0.5f;             // 原点を通る平面
	ground.size = 100.0f;        // 平面のサイズ
	ground.divisions = 36;     // グリッドの分割数

	// ===== スカイボックス初期化 =====
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

	// デバッグカメラ更新
	UpdateDebugCamera();

	// スカイボックス更新
	skybox->Update();

	// パーティクル更新
	if (particle) {
		particle->EmitByPresetName(emitPresetName, emitterTransform);
		particle->Update();
	}

	if (Input::GetInstance()->TriggerKey(DIK_G)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
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

	//skybox->Draw();

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

	// グリッド描画
	DrawLine::GetInstance()->DrawPlane(ground);

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

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 workPos = viewport->WorkPos;
	ImVec2 workSize = viewport->WorkSize;

	// レイアウト比率
	const float leftRatio = 0.25f; // 左カラムの幅
	const float rightRatio = 0.30f; // 右カラムの幅
	const float cameraRatio = 0.30f; // 左カラムの下段(カメラ)が占める高さ割合

	float leftWidth = workSize.x * leftRatio;
	float rightWidth = workSize.x * rightRatio;

	// 左カラムを上下に分割
	float cameraHeight = workSize.y * cameraRatio;
	float particleCtrlHeight = workSize.y - cameraHeight;

	// 左上(Particle Control)
	ImVec2 leftTopPos = workPos;
	ImVec2 leftTopSize = ImVec2(leftWidth, particleCtrlHeight);

	// 左下(Camera Control)
	ImVec2 leftBottomPos = ImVec2(workPos.x, workPos.y + particleCtrlHeight);
	ImVec2 leftBottomSize = ImVec2(leftWidth, cameraHeight);

	// 右カラム(プリセットエディタ)は全高
	ImVec2 rightPos = ImVec2(workPos.x + workSize.x - rightWidth, workPos.y);
	ImVec2 rightSize = ImVec2(rightWidth, workSize.y);

	ImGuiWindowFlags panelFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	//==========================
	// 左上：Particle Control
	//==========================
	ImGui::SetNextWindowPos(leftTopPos);
	ImGui::SetNextWindowSize(leftTopSize);
	if (ImGui::Begin(kWindowName_Particle, nullptr, panelFlags))
	{
		ImGui::Text("このシーンはパーティクルプリセットの編集＆プレビュー用です。");
		ImGui::Separator();

		ImGui::Text("Emit Transform");
		ImGui::DragFloat3("Position", &emitterTransform.translate.x, 0.1f);
		ImGui::DragFloat3("Rotation", &emitterTransform.rotate.x, 0.1f);
		ImGui::DragFloat3("Scale", &emitterTransform.scale.x, 0.1f, 0.0f, 100.0f);

		ImGui::Separator();

		ImGui::Text("Emit Preset Name");
		char buf[64]{};
		strncpy_s(buf, sizeof(buf), emitPresetName.c_str(), _TRUNCATE);
		if (ImGui::InputText("Preset Name", buf, sizeof(buf))) {
			emitPresetName = buf;
		}

		if (ImGui::Button("Emit Now")) {
			if (particle) {
				particle->EmitByPresetName(emitPresetName, emitterTransform);
			}
		}
		ImGui::Indent(10.0f);
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
			"・現在編集中のプリセットを1回だけ再生します\n"
			"・保存しなくても見た目をすぐ確認できます");
		ImGui::Unindent(10.0f);

		ImGui::Separator();

		if (ImGui::Button("Reload All Presets")) {
			if (particle) {
				particle->LoadAllPresets();
			}
		}
		ImGui::Indent(10.0f);
		ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f),
			"・Resources/Particle/*.json をすべて読み込み直します\n"
			"・外部ツールで編集した JSON も反映されます\n"
			"・プリセット一覧を最新状態に更新します");
		ImGui::Unindent(10.0f);
	}
	ImGui::End();

	//==========================
    // 左下：Camera Control
    //==========================
	ImGui::SetNextWindowPos(leftBottomPos);
	ImGui::SetNextWindowSize(leftBottomSize);
	if (ImGui::Begin(kWindowName_Camera, nullptr, panelFlags))
	{
		if (camera) {
			ImGui::Text("Camera Control");
			ImGui::Separator();

			// デバッグカメラON/OFF
			ImGui::Checkbox("Enable Debug Camera", &debugCameraEnabled_);
			/*ImGui::TextUnformatted("WASD + Mouse で移動/回転");
			ImGui::TextUnformatted("Mouse左クリックしながら回転");*/

			// debugCameraEnabled_ が true のときだけ表示
			if (debugCameraEnabled_) {
				ImGui::TextUnformatted(
					"操作方法:\n"
					"  左クリック中      : マウスで視点回転\n"
					"  右クリック中 + W  : 上に移動\n"
					"  右クリック中 + S  : 下に移動\n"
					"  W / S             : 前進 / 後退\n"
					"  A / D             : 左 / 右 移動"
				);
				ImGui::Separator();
			}

			// 手動で位置・回転をいじるスライダー（お好みで）
			Vector3 camPos = camera->GetTranslate();
			if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) {
				camera->SetTranslate(camPos);
			}

			Vector3 camRot = camera->GetRotate();
			if (ImGui::DragFloat3("Rotation", &camRot.x, 0.01f)) {
				camera->SetRotate(camRot);
			}
		}
		else {
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1),
				"Camera が nullptr です。");
		}
	}
	ImGui::End();


	//==========================
	// 右：Particle Preset Editor
	//==========================
	ImGui::SetNextWindowPos(rightPos);
	ImGui::SetNextWindowSize(rightSize);
	if (particle) {
		// 内部で ImGui::Begin("Particle Preset Editor") が呼ばれる
		particle->DrawImGuiParticlePresetEditor();
	}

	//==========================
	// 下：Console（固定）
	//==========================
	/*ImGui::SetNextWindowPos(bottomPos);
	ImGui::SetNextWindowSize(bottomSize);
	if (ImGui::Begin(kWindowName_Console, nullptr, panelFlags))
	{
		ImGui::Text("Console");
		ImGui::Separator();
		ImGui::Text("ここにログやデバッグ用の値を表示できます。");
	}

	ImGui::End();*/


	ImGui::PopStyleVar(); // WindowRounding 戻す
#endif
}

void ParticleEditorScene::UpdateDebugCamera()
{
	if (!debugCameraEnabled_ || !camera) {
		return;
	}

	Input* input = Input::GetInstance();

	constexpr float kDeltaTime = 1.0f / 60.0f;
	constexpr float kMoveSpeed = 0.5f;    // WASD移動速度（必要なら調整）
	constexpr float kRotateSpeed = 0.0003f;  // マウス回転速度（感度）

	Vector3 pos = camera->GetTranslate();
	Vector3 rot = camera->GetRotate();

	//=========================================================
	// 1. WASD 移動
	//=========================================================
	Vector3 move{};
	bool rightPressed = input->PushMouseButton(1);  // 1=右クリック

	// --- 右クリック押し中：W/S = 上下移動 ---
	if (rightPressed) {
		if (input->PushKey(DIK_W)) move.y += 1.0f;  // 上昇
		if (input->PushKey(DIK_S)) move.y -= 1.0f;  // 下降
	}
	// --- 通常：W/S = 前後移動 ---
	else {
		if (input->PushKey(DIK_W)) move.z += 1.0f;  // 前進
		if (input->PushKey(DIK_S)) move.z -= 1.0f;  // 後退
	}

	// A / D はどちらの場合も左右移動
	if (input->PushKey(DIK_D)) move.x += 1.0f;
	if (input->PushKey(DIK_A)) move.x -= 1.0f;

	//// E / Q で上下移動
	//if (input->PushKey(DIK_E)) move.y += 1.0f;
	//if (input->PushKey(DIK_Q)) move.y -= 1.0f;

	//=========================================================
	// 2. マウス視点回転
	//=========================================================
	if (input->PushMouseButton(0)) {     // 0 = 左ボタン
		POINT m = input->GetMouseDelta();
		rot.y += -m.x * kRotateSpeed;    // 左右
		rot.x += -m.y * kRotateSpeed;    // 上下

		const float kPitchLimit = 1.5f;
		rot.x = std::clamp(rot.x, -kPitchLimit, kPitchLimit);
	}


	//=========================================================
	// 3. ローカル移動 → ワールド座標へ変換
	//=========================================================
	if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f)
	{
		float cy = std::cos(rot.y);
		float sy = std::sin(rot.y);

		Vector3 forward{ sy, 0.0f, cy };
		Vector3 right{ cy, 0.0f, -sy };

		Vector3 worldMove{};
		worldMove = worldMove + forward * move.z;
		worldMove = worldMove + right * move.x;
		worldMove = worldMove + Vector3{ 0,1,0 } *move.y;

		pos = pos + worldMove * (kMoveSpeed);
	}

	//=========================================================
	// 4. カメラへ適用
	//=========================================================
	camera->SetTranslate(pos);
	camera->SetRotate(rot);
}


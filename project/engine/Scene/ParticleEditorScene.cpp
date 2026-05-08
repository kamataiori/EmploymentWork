#include "ParticleEditorScene.h"
#include <GlobalVariables.h>
#include <Input.h>
#include "SceneManager.h"

#include <SrvManager.h>
#include "PostEffectManager.h"
#include "engine/TimeManager.h"

#ifdef USE_IMGUI

#include <externals/imgui/imgui_internal.h>

#endif // USE_IMGUI

#ifdef USE_IMGUI
namespace
{
	void DrawCurve1DEditor(const char* title,
		Curve1D& curve,
		bool* openFlag)
	{
		if (!*openFlag) { return; }

		ImGui::SeparatorText(title);

		if (ImGui::Button("戻る")) {
			*openFlag = false;
			return;
		}

		ImGui::Checkbox("カーブを有効にする", &curve.enabled);

		if (!curve.enabled) {
			ImGui::TextUnformatted("有効にすると、NormalizedAge(0～1)に応じて値を補間します。");
			return;
		}

		// キーが無ければデフォルト(0→1 を1.0一定)を作る
		if (curve.keys.empty()) {
			curve.keys.push_back({ 0.0f, 1.0f });
			curve.keys.push_back({ 1.0f, 1.0f });
		}

		const float valueMin = 0.0f;
		const float valueMax = 5.0f;

		// 横幅を 1/3 : 2/3 に分割
		ImVec2 avail = ImGui::GetContentRegionAvail();
		float leftWidth = avail.x * (1.0f / 3.0f);
		float rightWidth = avail.x - leftWidth;

		// ===== 左：Curve Preview =====
		ImGui::BeginGroup();
		ImGui::Text("Curve Preview");

		// 残り高さをほぼ全部、グラフに使う
		float headerH = ImGui::GetTextLineHeightWithSpacing() * 2.0f; // 「Curve Preview」など
		float graphHeight = avail.y - headerH;
		if (graphHeight < 80.0f)  graphHeight = 80.0f;      // 最低 80px
		if (graphHeight > avail.y) graphHeight = avail.y;   // はみ出さないように

		ImVec2 canvasSize(leftWidth, graphHeight);

		// ここでマウス入力も取る
		ImGui::InvisibleButton("CurveCanvas", canvasSize);
		ImVec2 canvasMin = ImGui::GetItemRectMin();
		ImVec2 canvasMax = ImGui::GetItemRectMax();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// クリップ（線やポイントが外にはみ出さない）
		drawList->PushClipRect(canvasMin, canvasMax, true);

		// 背景
		drawList->AddRectFilled(canvasMin, canvasMax,
			IM_COL32(30, 30, 30, 255));
		drawList->AddRect(canvasMin, canvasMax,
			IM_COL32(100, 100, 100, 255));

		// グリッド線
		for (int i = 0; i <= 10; ++i) {
			float t = i / 10.0f;
			float x = ImLerp(canvasMin.x, canvasMax.x, t);
			drawList->AddLine(ImVec2(x, canvasMin.y),
				ImVec2(x, canvasMax.y),
				IM_COL32(60, 60, 60, 255));
		}
		for (int i = 0; i <= 5; ++i) {
			float v = valueMin + (valueMax - valueMin) * (i / 5.0f);
			float ny = (v - valueMin) / (valueMax - valueMin);
			float y = ImLerp(canvasMax.y, canvasMin.y, ny);
			drawList->AddLine(ImVec2(canvasMin.x, y),
				ImVec2(canvasMax.x, y),
				IM_COL32(60, 60, 60, 255));
		}

		// カーブライン
		const int sampleCount = 64;
		for (int i = 0; i < sampleCount - 1; ++i) {
			float t0 = (float)i / (float)(sampleCount - 1);
			float t1 = (float)(i + 1) / (float)(sampleCount - 1);
			float v0 = curve.Evaluate(t0);
			float v1 = curve.Evaluate(t1);

			float n0 = (v0 - valueMin) / (valueMax - valueMin);
			float n1 = (v1 - valueMin) / (valueMax - valueMin);
			ImVec2 p0 = ImVec2(ImLerp(canvasMin.x, canvasMax.x, t0),
				ImLerp(canvasMax.y, canvasMin.y, n0));
			ImVec2 p1 = ImVec2(ImLerp(canvasMin.x, canvasMax.x, t1),
				ImLerp(canvasMax.y, canvasMin.y, n1));
			drawList->AddLine(p0, p1, IM_COL32(255, 200, 50, 255), 2.0f);
		}

		// キーのドラッグ
		static int  sActiveKey = -1;
		static bool sDragging = false;

		ImGuiIO& io = ImGui::GetIO();
		bool hoveredCanvas = ImGui::IsItemHovered(); // InvisibleButton に対して

		for (size_t i = 0; i < curve.keys.size(); ++i) {
			const auto& key = curve.keys[i];

			float nx = key.t;
			float ny = (key.v - valueMin) / (valueMax - valueMin);
			ny = std::clamp(ny, 0.0f, 1.0f);

			ImVec2 p = ImVec2(ImLerp(canvasMin.x, canvasMax.x, nx),
				ImLerp(canvasMax.y, canvasMin.y, ny));

			float radius = 5.0f;
			ImU32 col = IM_COL32(200, 200, 255, 255);
			if ((int)i == sActiveKey) {
				col = IM_COL32(255, 255, 100, 255);
				radius = 7.0f;
			}

			drawList->AddCircleFilled(p, radius, col, 16);

			if (hoveredCanvas && ImGui::IsMouseClicked(0)) {
				float dx = io.MousePos.x - p.x;
				float dy = io.MousePos.y - p.y;
				float distSqr = dx * dx + dy * dy;
				if (distSqr <= (radius * radius * 4.0f)) {
					sActiveKey = (int)i;
					sDragging = true;
				}
			}
		}

		if (sDragging && sActiveKey >= 0 &&
			sActiveKey < (int)curve.keys.size())
		{
			if (ImGui::IsMouseDown(0)) {
				float t = (io.MousePos.x - canvasMin.x) /
					(canvasMax.x - canvasMin.x);
				float ny = (io.MousePos.y - canvasMax.y) /
					(canvasMin.y - canvasMax.y); // y 反転

				t = std::clamp(t, 0.0f, 1.0f);
				ny = std::clamp(ny, 0.0f, 1.0f);

				float v = valueMin + (valueMax - valueMin) * ny;
				v = std::clamp(v, valueMin, valueMax);

				float tMin = 0.0f;
				float tMax = 1.0f;
				if (sActiveKey > 0) {
					tMin = curve.keys[sActiveKey - 1].t + 0.001f;
				}
				if (sActiveKey + 1 < (int)curve.keys.size()) {
					tMax = curve.keys[sActiveKey + 1].t - 0.001f;
				}
				t = std::clamp(t, tMin, tMax);

				curve.keys[sActiveKey].t = t;
				curve.keys[sActiveKey].v = v;
			}
			else {
				sDragging = false;
				sActiveKey = -1;
			}
		}

		drawList->PopClipRect();
		ImGui::EndGroup();

		// ===== 右：Key Data =====
		ImGui::SameLine();
		ImGui::BeginGroup();

		ImGui::SeparatorText("Key Data (t, v)");

		if (ImGui::Button("キーを追加")) {
			float t = 0.5f;
			float v = 1.0f;
			if (!curve.keys.empty()) {
				t = curve.keys.back().t;
				v = curve.keys.back().v;
				t += 0.1f;
				if (t > 1.0f) t = 1.0f;
			}
			curve.keys.push_back({ t, v });
		}
		ImGui::SameLine();
		if (ImGui::Button("最後のキーを削除") && curve.keys.size() > 2) {
			curve.keys.pop_back();
		}

		ImGui::Spacing();

		// DragFloat でコンパクトに表示
		for (size_t i = 0; i < curve.keys.size(); ++i) {
			ImGui::PushID((int)i);

			ImGui::Text("%d", (int)i);
			ImGui::SameLine();

			float half = rightWidth * 0.4f;
			ImGui::SetNextItemWidth(half);
			ImGui::DragFloat("t", &curve.keys[i].t,
				0.01f, 0.0f, 1.0f, "t=%.2f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(half);
			ImGui::DragFloat("v", &curve.keys[i].v,
				0.01f, valueMin, valueMax, "v=%.2f");

			ImGui::PopID();
		}

		std::sort(curve.keys.begin(), curve.keys.end(),
			[](const auto& a, const auto& b) {
				return a.t < b.t;
			});

		ImGui::EndGroup();
	}

} // namespace
#endif


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
	particle->Initialize(VertexDataType::Plane);
	particle->SetCamera(camera.get());

	// 起動時に一括プリセット読み込み（Resources/Particle/*.json）
	particle->LoadAllPresets();

	// エミッターのデフォルト値（原点、スケール1）
	emitterTransform.scale = { 1.0f, 1.0f, 1.0f };
	emitterTransform.rotate = { 0.0f, 0.0f, 0.0f };
	emitterTransform.translate = { 0.0f, 0.0f, 1.0f };

	emitPresetName = "NewParticle"; // プリセットエディタのデフォルト名と合わせておく想定

	// ===== System/Emitter 初期カード =====
	niagaraSystems_.clear();
	niagaraEmitters_.clear();

	// ===== 保存済み System JSON を全部読み込む =====
	if (particle) {
		particle->LoadAllSystems();
	}

	// ===== 読み込んだ System を UI 側にも反映 =====
	if (particle) {
		std::vector<std::string> loadedNames = particle->GetAllSystemNames();

		float yOffset = 40.0f;
		for (const std::string& sysName : loadedNames) {
			NiagaraSystemUI sysUI{};
			sysUI.name = sysName;
			sysUI.posX = 40.0f;
			sysUI.posY = yOffset;
			sysUI.width = 140.0f;
			sysUI.height = 120.0f;
			sysUI.playing = false;
			sysUI.loop = false;
			sysUI.emitInterval = 0.2f;
			sysUI.emitTimer = 0.0f;

			// ParticleSystem 側から設定を取り出して UI に反映
			ParticleSystem* sys = particle->FindSystem(sysName);
			if (sys) {
				sysUI.loop = sys->IsLoop();
				sysUI.systemDuration = sys->GetDuration();

				// pendingSettings_ から emitterEntries を作る
				for (const auto& p : sys->GetPendingSettings()) {
					NiagaraSystemUI::EmitterEntryUI ent;
					ent.presetName = p.presetName;
					ent.startTime = p.startTime;
					ent.duration = p.duration;
					ent.autoPlay = p.autoPlay;
					sysUI.emitterEntries.push_back(ent);
				}
			}

			niagaraSystems_.push_back(sysUI);
			yOffset += 140.0f;  // 次の System カードを下にずらす
		}
	}

	// ===== JSON が一個も読まれなかった場合のフォールバック =====
	// （既存コード：NS_Sample のデフォルトカードを作る）
	if (niagaraSystems_.empty()) {
		NiagaraSystemUI sys{};
		sys.name = "NS_Sample";
		sys.posX = 40.0f;
		sys.posY = 40.0f;
		sys.width = 140.0f;
		sys.height = 120.0f;
		sys.playing = false;
		sys.loop = false;
		sys.emitInterval = 0.2f;
		sys.emitTimer = 0.0f;
		niagaraSystems_.push_back(sys);
	}

	systemNameCounter_ = static_cast<int>(niagaraSystems_.size()) + 1;
	selectedSystemIndex_ = niagaraSystems_.empty() ? -1 : 0;

	// ===== Emitter カード（既存コード） =====
	{
		NiagaraEmitterUI em{};
		em.name = "NE_Sample";
		em.presetName = em.name;
		if (!niagaraSystems_.empty()) {
			em.systemName = niagaraSystems_.front().name;
		}
		emitPresetName = em.presetName;
		em.posX = 280.0f;
		em.posY = 80.0f;
		em.width = 160.0f;
		em.height = 260.0f;
		niagaraEmitters_.push_back(em);
		emitterNameCounter_ = 2;
		selectedEmitterIndex_ = 0;
	}


#ifdef USE_IMGUI
	// 旧ドッキング設定はもう使わないので消してOK
	// AddRightDockWindow(kWindowName_Particle);
	// AddRightDockWindow(kWindowName_Preset);
#endif
}

void ParticleEditorScene::Finalize()
{
}

void ParticleEditorScene::Update()
{
	// deltaTime
	float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

	// カメラ更新
	if (camera) camera->Update();

	// デバッグカメラ更新
	UpdateDebugCamera();

	// スカイボックス更新
	skybox->Update();

	// パーティクル更新
	if (particle) {

		// 位置や寿命などの更新
		particle->Update();
	}

	// ===== System の自動再生 =====
	if (particle) {
		for (auto& sys : niagaraSystems_) {
			if (!sys.playing) {
				continue;
			}
			if (sys.name.empty()) {
				continue;
			}

			// System の再生管理は ParticleSystem 側（startTime/duration）に任せる
			//   ここでは「再生開始のトリガー」だけを担当する
			// Emit 間隔が 0 以下なら「押した瞬間に1回だけ Play」
			if (sys.emitInterval <= 0.0f) {
				particle->EmitSystemByName(sys.name, emitterTransform);

				if (!sys.loop) {
					// 1回再生なら即座に playing を落とす
					// System 側の自然終了を待たず、UI として停止状態に
					// System のパーティクルが消えるまで見た目は動き続ける
					sys.playing = false;
				}
				// Loop の場合は System 側の loop_ 設定に任せてもよいし、
				// ここで emitTimer を使って再発火制御してもよい,今回は前者
			}
			else {
				// タイマー減算して周期的に Emit
				sys.emitTimer -= dt;
				if (sys.emitTimer <= 0.0f) {
					particle->EmitSystemByName(sys.name, emitterTransform);

					if (sys.loop) {
						sys.emitTimer += sys.emitInterval;
					}
					else {
						sys.playing = false;
					}
				}
			}
		}
	}

	// デバッグ
	Debug();
}

void ParticleEditorScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// 背景スプライトあればここで
}

void ParticleEditorScene::Draw()
{
	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// 3Dオブジェクトがあればここで

	// アニメーションオブジェクトの描画前処理
	Skinning::GetInstance()->CommonSetting();

	// アニメーションオブジェクトがあればここで

	// グリッド描画
	DrawLine::GetInstance()->DrawPlane(ground);
}

void ParticleEditorScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// 前景スプライトがあればここで

	// パーティクル描画
	if (particle) {
		particle->Draw();
	}
}

void ParticleEditorScene::Debug()
{
#ifdef USE_IMGUI

	// 背景を不透明にする
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 workPos = viewport->WorkPos;
	ImVec2 workSize = viewport->WorkSize;

	const float totalW = workSize.x;
	const float totalH = workSize.y;

	// 安全のため比率をクランプ
	const float kMinRatio = 0.15f;
	ratioLeftRight_ = std::clamp(ratioLeftRight_, kMinRatio, 1.0f - kMinRatio);
	ratioTopBottom_ = std::clamp(ratioTopBottom_, kMinRatio, 1.0f - kMinRatio);
	ratioSceneCanvas_ = std::clamp(ratioSceneCanvas_, kMinRatio, 1.0f - kMinRatio);

	// ===== 比率 → 実際のサイズに変換 =====
	float leftWidth = totalW * ratioLeftRight_;
	float inspectorWidth = totalW - leftWidth;

	float topHeight = totalH * ratioTopBottom_;
	float bottomHeight = totalH - topHeight;

	float sceneWidth = leftWidth * ratioSceneCanvas_;
	float canvasWidth = leftWidth - sceneWidth;

	auto SafeSize = [](float w, float h) -> ImVec2 {
		if (w < 1.0f) w = 1.0f;
		if (h < 1.0f) h = 1.0f;
		return ImVec2(w, h);
		};

	// ===== 各パネルの位置・サイズ =====
	// 左上：SceneView
	ImVec2 scenePos = workPos;
	ImVec2 sceneSize = SafeSize(sceneWidth, topHeight);

	// 中央上：Niagara Canvas
	ImVec2 canvasPos = ImVec2(workPos.x + sceneWidth, workPos.y);
	ImVec2 canvasSize = SafeSize(canvasWidth, topHeight);

	// 右：Inspector（上下フル）
	ImVec2 inspectorPos = ImVec2(workPos.x + leftWidth, workPos.y);
	ImVec2 inspectorSize = SafeSize(inspectorWidth, totalH);

	// 下：Camera / Curve（右の Inspector は含めない）
	ImVec2 bottomPos = ImVec2(workPos.x, workPos.y + topHeight);
	ImVec2 bottomSize = SafeSize(leftWidth, bottomHeight);

	ImGuiWindowFlags panelFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	// -----------------------------
	// 1) 各パネル描画
	// -----------------------------

	if (showNiagaraUI_) {
		DrawSceneViewPanel(scenePos, sceneSize, static_cast<int>(panelFlags));
		DrawNiagaraCanvas(canvasPos, canvasSize, static_cast<int>(panelFlags));
		DrawNiagaraInspector(inspectorPos, inspectorSize, static_cast<int>(panelFlags));
	}

	if (curveEditorMode_ != CurveEditorMode::None && curveEditorTarget_) {
		DrawCurveEditorPanel(bottomPos, bottomSize, static_cast<int>(panelFlags));
	}
	else {
		DrawCameraControlPanel(bottomPos, bottomSize, static_cast<int>(panelFlags));
	}

	ImGui::PopStyleVar();
	// ここで元のスタイルに戻す
	ImGui::PopStyleColor(3);

	// -----------------------------
	// 2) スプリッタ（境界のドラッグ処理）
	// -----------------------------
	if (showNiagaraUI_)
	{
		ImDrawList* dl = ImGui::GetForegroundDrawList();
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 mousePos = io.MousePos;

		const float thickness = 4.0f;

		static bool draggingSceneCanvas = false;
		static bool draggingLeftRight = false;
		static bool draggingTopBottom = false;

		auto VerticalSplitter = [&](float x, float top, float bottom,
			float totalWidth, float& ratio,
			bool& draggingFlag)
			{
				float half = thickness * 0.5f;
				bool hovered =
					(mousePos.x >= x - half && mousePos.x <= x + half &&
						mousePos.y >= top && mousePos.y <= bottom);

				if (hovered || draggingFlag) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				}

				if (hovered && ImGui::IsMouseClicked(0)) {
					draggingFlag = true;
				}
				if (!ImGui::IsMouseDown(0)) {
					draggingFlag = false;
				}

				if (draggingFlag) {
					float delta = io.MouseDelta.x;
					ratio += delta / totalWidth;
					ratio = std::clamp(ratio, 0.15f, 1.0f - 0.15f);
				}

				dl->AddLine(ImVec2(x, top), ImVec2(x, bottom),
					IM_COL32(180, 180, 180, hovered ? 255 : 160), 1.0f);
			};

		auto HorizontalSplitter = [&](float y, float left, float right,
			float totalHeight, float& ratio,
			bool& draggingFlag)
			{
				float half = thickness * 0.5f;
				bool hovered =
					(mousePos.y >= y - half && mousePos.y <= y + half &&
						mousePos.x >= left && mousePos.x <= right);

				if (hovered || draggingFlag) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
				}

				if (hovered && ImGui::IsMouseClicked(0)) {
					draggingFlag = true;
				}
				if (!ImGui::IsMouseDown(0)) {
					draggingFlag = false;
				}

				if (draggingFlag) {
					float delta = io.MouseDelta.y;
					ratio += delta / totalHeight;
					ratio = std::clamp(ratio, 0.15f, 1.0f - 0.15f);
				}

				dl->AddLine(ImVec2(left, y), ImVec2(right, y),
					IM_COL32(180, 180, 180, hovered ? 255 : 160), 1.0f);
			};

		// Splitters…
		{
			float splitX = workPos.x + sceneWidth;
			float top = workPos.y;
			float bottom = workPos.y + topHeight;
			VerticalSplitter(splitX, top, bottom, leftWidth, ratioSceneCanvas_, draggingSceneCanvas);
		}

		{
			float splitX = workPos.x + leftWidth;
			float top = workPos.y;
			float bottom = workPos.y + totalH;
			VerticalSplitter(splitX, top, bottom, totalW, ratioLeftRight_, draggingLeftRight);
		}

		{
			float splitY = workPos.y + topHeight;
			float left = workPos.x;
			float right = workPos.x + leftWidth;
			HorizontalSplitter(splitY, left, right, totalH, ratioTopBottom_, draggingTopBottom);
		}
	}


#endif // USE_IMGUI
}

void ParticleEditorScene::UpdateDebugCamera()
{
	if (!debugCameraEnabled_ || !camera) {
		return;
	}

	Input* input = Input::GetInstance();

	float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
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

		pos = pos + worldMove * (kMoveSpeed * dt * 60.0f);
	}

	//=========================================================
	// 4. カメラへ適用
	//=========================================================
	camera->SetTranslate(pos);
	camera->SetRotate(rot);
}

#ifdef USE_IMGUI
//--------------------------------------------------
// 左上：SceneView
//--------------------------------------------------
void ParticleEditorScene::DrawSceneViewPanel(const ImVec2& pos, const ImVec2& size, int panelFlags)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGuiWindowFlags flags = static_cast<ImGuiWindowFlags>(panelFlags);

	if (!ImGui::Begin("SceneView", nullptr, flags))
	{
		ImGui::End();
		return;
	}

	// MyGame::DrawCenterPanel と同じ描画方法
	ImTextureID textureID = (ImTextureID)SrvManager::GetInstance()
		->GetGPUDescriptorHandle(PostEffectManager::GetInstance()->GetSrvIndex()).ptr;

	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImGui::Image(textureID, avail);

	ImGui::End();
}

//--------------------------------------------------
// 中央：Niagara Canvas
//--------------------------------------------------
void ParticleEditorScene::DrawNiagaraCanvas(const ImVec2& pos, const ImVec2& size, int panelFlags)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	if (ImGui::Begin("Niagara Canvas", nullptr, panelFlags))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImVec2 avail = ImGui::GetContentRegionAvail();

		// 背景
		drawList->AddRectFilled(origin,
			ImVec2(origin.x + avail.x, origin.y + avail.y),
			IM_COL32(30, 30, 35, 255));

		// 右クリックメニューで NS / NE / ペースト を追加
		if (ImGui::BeginPopupContextWindow("NiagaraCanvasContext",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			ImVec2 mouse = ImGui::GetMousePos();
			ImVec2 local(mouse.x - origin.x, mouse.y - origin.y);

			if (ImGui::MenuItem("Add System (NS_*)")) {
				NiagaraSystemUI sys{};
				sys.name = "NS_" + std::to_string(systemNameCounter_++);
				sys.posX = local.x;
				sys.posY = local.y;
				sys.width = 140.0f;
				sys.height = 120.0f;
				niagaraSystems_.push_back(sys);
			}

			if (ImGui::MenuItem("Add Emitter (NE_*)")) {
				NiagaraEmitterUI em{};
				em.name = "NE_" + std::to_string(emitterNameCounter_++);
				em.presetName = emitPresetName;   // いま編集中のプリセット名をデフォルトで付ける
				// 選択中の System があれば、その名前を System 名としてセット
				if (selectedSystemIndex_ >= 0 &&
					selectedSystemIndex_ < static_cast<int>(niagaraSystems_.size())) {
					em.systemName = niagaraSystems_[selectedSystemIndex_].name;
				}

				em.posX = local.x;
				em.posY = local.y;
				em.width = 170.0f;
				em.height = 260.0f;
				em.selectedModuleIndex = 0;
				niagaraEmitters_.push_back(em);
			}

			// ペースト
			if (ImGui::MenuItem("Paste Emitter", nullptr, false, hasEmitterClipboard_)) {
				if (hasEmitterClipboard_) {
					NiagaraEmitterUI em = emitterClipboard_;
					em.posX = local.x;
					em.posY = local.y;
					niagaraEmitters_.push_back(em);
				}
			}

			ImGui::EndPopup();
		}

		// ===== System カード描画 & ドラッグ =====
		for (int i = 0; i < (int)niagaraSystems_.size(); ++i) {
			NiagaraSystemUI& sys = niagaraSystems_[i];
			ImVec2 p0(origin.x + sys.posX, origin.y + sys.posY);
			ImVec2 p1(p0.x + sys.width, p0.y + sys.height);

			ImGui::PushID(i);
			ImGui::SetCursorScreenPos(p0);
			ImGui::InvisibleButton("SystemCard", ImVec2(sys.width, sys.height));
			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			// ドラッグ移動
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				ImVec2 delta = ImGui::GetIO().MouseDelta;
				sys.posX += delta.x;
				sys.posY += delta.y;
			}

			ImU32 col = (selectedSystemIndex_ == i && selectedEmitterIndex_ < 0)
				? IM_COL32(60, 140, 220, 255)
				: IM_COL32(40, 80, 130, 255);

			drawList->AddRectFilled(p0, p1, col, 6.0f);
			drawList->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 6.0f);
			drawList->AddText(ImVec2(p0.x + 8, p0.y + 8),
				IM_COL32(255, 255, 255, 255),
				sys.name.c_str());

			if (hovered) {
				drawList->AddRect(p0, p1, IM_COL32(255, 255, 0, 255), 6.0f);
			}
			if (clicked) {
				selectedSystemIndex_ = i;
				selectedEmitterIndex_ = -1;
			}

			ImGui::PopID();
		}

		// ====== モジュールの表示情報（カード内用） ======
		struct ModuleDesc {
			const char* label;
			ImU32       color;  // 左のアイコン色
		};

		static const ModuleDesc kEmitterModules[] = {
			{ "Name",            IM_COL32(255, 255, 255, 255) }, // 0
			{ "Emitter Settings",IM_COL32(255, 150, 70, 255) },   // 1
			{ "Emitter Spawn",   IM_COL32(255, 120, 40, 255) },   // 2
			{ "Emitter Update",  IM_COL32(255, 120, 40, 255) },   // 3
			{ "Particle Spawn",  IM_COL32(120, 220, 80, 255) },   // 4
			{ "Particle Update", IM_COL32(120, 220, 80, 255) },   // 5
			{ "Render",          IM_COL32(220, 80, 80, 255) },    // 6
		};
		constexpr int kModuleCount = sizeof(kEmitterModules) / sizeof(ModuleDesc);

		int emitterToDelete = -1;

		// ===== Emitter カード描画 & ドラッグ & モジュール行 =====
		for (int i = 0; i < (int)niagaraEmitters_.size(); ++i) {
			NiagaraEmitterUI& em = niagaraEmitters_[i];
			ImVec2 p0(origin.x + em.posX, origin.y + em.posY);
			ImVec2 p1(p0.x + em.width, p0.y + em.height);

			ImGui::PushID(1000 + i);
			ImGui::SetCursorScreenPos(p0);
			ImGui::InvisibleButton("EmitterCard", ImVec2(em.width, em.height));
			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			// ドラッグ移動
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				ImVec2 delta = ImGui::GetIO().MouseDelta;
				em.posX += delta.x;
				em.posY += delta.y;
			}

			// 右クリックメニュー（コピー / 削除）
			if (ImGui::BeginPopupContextItem("EmitterCardContext")) {
				if (ImGui::MenuItem("コピー")) {
					emitterClipboard_ = em;
					hasEmitterClipboard_ = true;
				}
				if (ImGui::MenuItem("削除")) {
					emitterToDelete = i;
				}
				ImGui::EndPopup();
			}

			ImU32 col = (selectedEmitterIndex_ == i)
				? IM_COL32(220, 140, 60, 255)
				: IM_COL32(150, 90, 40, 255);

			// カード本体
			drawList->AddRectFilled(p0, p1, col, 6.0f);
			drawList->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 6.0f);

			// 名前ヘッダ
			ImVec2 namePos(p0.x + 10.0f, p0.y + 6.0f);
			drawList->AddText(namePos,
				IM_COL32(255, 255, 255, 255),
				em.name.c_str());

			// モジュール行の開始位置
			float headerHeight = 26.0f;
			float moduleHeight = 22.0f;
			float innerLeft = p0.x + 6.0f;
			float innerRight = p1.x - 6.0f;

			ImVec2 mouse = ImGui::GetIO().MousePos;

			for (int m = 0; m < kModuleCount; ++m) {
				float y0 = p0.y + headerHeight + moduleHeight * m;
				float y1 = y0 + moduleHeight - 2.0f;

				ImVec2 rm0(innerLeft, y0);
				ImVec2 rm1(innerRight, y1);

				bool rowHover =
					mouse.x >= rm0.x && mouse.x <= rm1.x &&
					mouse.y >= rm0.y && mouse.y <= rm1.y;

				// 選択＆ホバー色
				ImU32 rowCol;
				if (selectedEmitterIndex_ == i && em.selectedModuleIndex == m) {
					rowCol = IM_COL32(60, 60, 60, 255);
				}
				else if (rowHover) {
					rowCol = IM_COL32(50, 50, 50, 255);
				}
				else {
					rowCol = IM_COL32(40, 40, 40, 255);
				}

				// 行の背景
				drawList->AddRectFilled(rm0, rm1, rowCol, 4.0f);

				// 左の小さいアイコン
				ImVec2 icon0(rm0.x + 3.0f, rm0.y + 3.0f);
				ImVec2 icon1(icon0.x + 10.0f, rm1.y - 3.0f);
				drawList->AddRectFilled(icon0, icon1,
					kEmitterModules[m].color, 2.0f);

				// ラベル
				drawList->AddText(ImVec2(icon1.x + 6.0f, rm0.y + 4.0f),
					IM_COL32(220, 220, 220, 255),
					kEmitterModules[m].label);

				// 行クリックでモジュール選択
				if (rowHover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					selectedEmitterIndex_ = i;
					selectedSystemIndex_ = -1;
					em.selectedModuleIndex = m;
					emitPresetName = em.presetName;
				}
			}

			if (hovered) {
				drawList->AddRect(p0, p1,
					IM_COL32(255, 255, 0, 255), 6.0f);
			}
			if (clicked && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				selectedEmitterIndex_ = i;
				selectedSystemIndex_ = -1;
				emitPresetName = em.presetName;
			}

			ImGui::PopID();
		}

		// 削除反映
		if (emitterToDelete >= 0 &&
			emitterToDelete < (int)niagaraEmitters_.size())
		{
			niagaraEmitters_.erase(niagaraEmitters_.begin() + emitterToDelete);

			if (selectedEmitterIndex_ == emitterToDelete) {
				selectedEmitterIndex_ = -1;
			}
			else if (selectedEmitterIndex_ > emitterToDelete) {
				selectedEmitterIndex_--;
			}
		}
	}
	ImGui::End();
}

//--------------------------------------------------
// 右：Emitter Inspector
//--------------------------------------------------
void ParticleEditorScene::DrawNiagaraInspector(const ImVec2& pos, const ImVec2& size, int panelFlags)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGuiWindowFlags flags = static_cast<ImGuiWindowFlags>(panelFlags);

	if (!ImGui::Begin("Emitter Inspector", nullptr, flags))
	{
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
		"選択中のSystem / Emitter の詳細");
	ImGui::Separator();

	// 選択中の Emitter を取得
	NiagaraEmitterUI* emitterUI = nullptr;
	if (selectedEmitterIndex_ >= 0 &&
		selectedEmitterIndex_ < static_cast<int>(niagaraEmitters_.size())) {
		emitterUI = &niagaraEmitters_[selectedEmitterIndex_];
	}

	// 選択中の System を取得
	NiagaraSystemUI* systemUI = nullptr;
	if (selectedSystemIndex_ >= 0 &&
		selectedSystemIndex_ < static_cast<int>(niagaraSystems_.size())) {
		systemUI = &niagaraSystems_[selectedSystemIndex_];
	}

	// ---- Emitter が未選択の場合 ----
	if (!emitterUI) {

		if (systemUI) {
			ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
				"System Inspector");
			ImGui::Separator();

			// --- System Name 編集欄 ---
			{
				char buf[64];
				strncpy_s(buf, sizeof(buf), systemUI->name.c_str(), _TRUNCATE);

				if (ImGui::InputText("System Name", buf, sizeof(buf))) {
					std::string oldName = systemUI->name;
					std::string newName = buf;

					if (!newName.empty() && newName != oldName) {
						systemUI->name = newName;
						for (auto& e : niagaraEmitters_) {
							if (e.systemName == oldName) {
								e.systemName = newName;
							}
						}
						if (particle) {
							particle->RenameSystem(oldName, newName);
						}
					}
				}
			}

			if (ImGui::Button("Save System JSON")) {
				if (particle) {
					// ===== Phase2 追加: UI の編集内容を ParticleSystem に反映してから保存 =====
					ParticleSystem* sys = particle->FindSystem(systemUI->name);
					if (sys) {
						// System 全体の duration / loop を反映
						sys->SetLoop(systemUI->loop);
						sys->SetDuration(systemUI->systemDuration);

						// 各 Emitter の startTime/duration/autoPlay を反映
						// 既存 Emitter があれば EmitterEntry を、なければ pendingSettings_ を更新
						sys->ClearPendingSettings();
						for (const auto& uiEntry : systemUI->emitterEntries) {
							// pendingSettings_ に積む（次回 EmitSystem 時に Emitter 作成時に反映）
							ParticleSystem::PendingEmitterSetting p;
							p.presetName = uiEntry.presetName;
							p.startTime = uiEntry.startTime;
							p.duration = uiEntry.duration;
							p.autoPlay = uiEntry.autoPlay;
							sys->GetPendingSettings().push_back(p);

							// 既に作成済みの Emitter なら EmitterEntry も直接更新
							auto* entry = sys->FindEntryByPresetName(uiEntry.presetName);
							if (entry) {
								entry->startTime = uiEntry.startTime;
								entry->duration = uiEntry.duration;
								entry->autoPlay = uiEntry.autoPlay;
							}
						}
					}

					particle->SaveSystemToJson(systemUI->name);
				}
			}

			ImGui::Separator();

			// ===== System 全体の設定 =====
			ImGui::Checkbox("Loop 再生", &systemUI->loop);
			ImGui::DragFloat("System Duration (sec)", &systemUI->systemDuration, 0.05f, 0.0f, 60.0f);
			ImGui::DragFloat("Emit 間隔 (sec)", &systemUI->emitInterval, 0.01f, 0.0f, 10.0f);

			// 再生 / 停止ボタン
			if (!systemUI->playing) {
				if (ImGui::Button("再生 (Play)")) {
					systemUI->playing = true;
					systemUI->emitTimer = 0.0f;
				}
			}
			else {
				if (ImGui::Button("停止 (Stop)")) {
					systemUI->playing = false;
				}
			}

			ImGui::Separator();

			// ===== タイムラインバー描画 =====
			{
				// タイムラインの全体長さを決める
				// systemDuration が 0 の場合は、Emitter の startTime+duration の最大値で代替
				float timelineLength = systemUI->systemDuration;
				if (timelineLength <= 0.0f) {
					for (const auto& ent : systemUI->emitterEntries) {
						float endT = ent.startTime + (ent.duration > 0.0f ? ent.duration : 1.0f);
						if (endT > timelineLength) timelineLength = endT;
					}
					if (timelineLength <= 0.0f) timelineLength = 2.0f;  // デフォルト2秒
				}

				ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
					"Timeline (Total: %.2f sec)", timelineLength);

				// タイムライン領域のサイズ
				const float timelineHeight = 24.0f * (systemUI->emitterEntries.size() + 1);  // 目盛り+各Emitter行
				const float padding = 8.0f;

				ImVec2 timelinePos = ImGui::GetCursorScreenPos();
				ImVec2 timelineSize = ImVec2(ImGui::GetContentRegionAvail().x - padding, timelineHeight);

				// 領域確保（後続の UI が下に並ぶように）
				ImGui::Dummy(timelineSize);

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				// 背景
				drawList->AddRectFilled(
					timelinePos,
					ImVec2(timelinePos.x + timelineSize.x, timelinePos.y + timelineSize.y),
					IM_COL32(40, 40, 50, 255), 4.0f);

				// 時刻目盛り（1秒ごと）
				const float pixelPerSec = timelineSize.x / timelineLength;
				const int maxTick = static_cast<int>(timelineLength) + 1;
				for (int i = 0; i <= maxTick; ++i) {
					float t = static_cast<float>(i);
					if (t > timelineLength) break;

					float xPos = timelinePos.x + t * pixelPerSec;
					// 縦線
					drawList->AddLine(
						ImVec2(xPos, timelinePos.y),
						ImVec2(xPos, timelinePos.y + timelineSize.y),
						IM_COL32(80, 80, 90, 255), 1.0f);
					// 数字（先頭の0sは省略）
					char tickLabel[16];
					snprintf(tickLabel, sizeof(tickLabel), "%.0fs", t);
					drawList->AddText(
						ImVec2(xPos + 2.0f, timelinePos.y + 2.0f),
						IM_COL32(160, 160, 170, 255), tickLabel);
				}

				// Emitter ごとのバー
				const float rowHeight = 22.0f;
				const float rowOffsetY = 22.0f;  // 目盛り行の高さ
				const float barPadding = 2.0f;

				for (size_t i = 0; i < systemUI->emitterEntries.size(); ++i) {
					const auto& ent = systemUI->emitterEntries[i];

					float startX = timelinePos.x + ent.startTime * pixelPerSec;

					// duration < 0 ならタイムライン端まで伸ばす
					float endTime = ent.duration > 0.0f
						? (ent.startTime + ent.duration)
						: timelineLength;
					float endX = timelinePos.x + endTime * pixelPerSec;

					// タイムライン外にはみ出ないようにクランプ
					float maxX = timelinePos.x + timelineSize.x;
					if (endX > maxX) endX = maxX;
					if (startX > maxX) startX = maxX;

					float yTop = timelinePos.y + rowOffsetY + i * rowHeight + barPadding;
					float yBottom = yTop + rowHeight - barPadding * 2.0f;

					// Emitter ごとに異なる色（HSV 色相をずらす）
					float hue = static_cast<float>(i) * 0.15f;
					ImVec4 col4 = ImColor::HSV(hue, 0.6f, 0.8f);
					ImU32 barColor = ImGui::ColorConvertFloat4ToU32(col4);

					// バー描画
					if (endX > startX) {
						drawList->AddRectFilled(
							ImVec2(startX, yTop),
							ImVec2(endX, yBottom),
							barColor, 3.0f);
					}
					else {
						// duration=0 で表示できない場合の小さなマーカー
						drawList->AddRectFilled(
							ImVec2(startX - 2.0f, yTop),
							ImVec2(startX + 2.0f, yBottom),
							barColor, 2.0f);
					}

					// プリセット名をバーの中（または左）に表示
					ImVec2 textPos = ImVec2(startX + 4.0f, yTop + 2.0f);
					drawList->AddText(textPos, IM_COL32(255, 255, 255, 255),
						ent.presetName.c_str());
				}

				// 現在の再生位置カーソル（System が Playing 中のみ）
				if (systemUI->playing && timelineLength > 0.0f) {
					// emitTimer をベースに簡易表示
					// ※正確な再生位置は ParticleSystem 側 time_ を使うべきだが、
					//   まずは UI 側のタイマーで代用する
					float currentT = systemUI->emitTimer;
					// emitTimer が「カウントダウン」式の場合は逆算が必要
					// ここでは「経過時間」と仮定した実装にする
					if (currentT < 0.0f) currentT = 0.0f;
					if (currentT > timelineLength) currentT = timelineLength;

					float cursorX = timelinePos.x + currentT * pixelPerSec;
					drawList->AddLine(
						ImVec2(cursorX, timelinePos.y),
						ImVec2(cursorX, timelinePos.y + timelineSize.y),
						IM_COL32(255, 80, 80, 255), 2.0f);
				}
			}

			ImGui::Separator();

			// ===== System に登録されたプリセット一覧と時間編集 =====
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Emitters in System");
			ImGui::Separator();

			// ParticleSystem 側のプリセット名一覧と UI 側の emitterEntries を同期
			// (presets が増減した場合に UI 側も追従させる)
			if (particle) {
				const auto* presetNames = particle->GetSystemPresets(systemUI->name);
				if (presetNames) {
					// 1 ParticleSystem 側にあって UI 側にないプリセットを追加
					for (const auto& presetName : *presetNames) {
						bool exists = false;
						for (const auto& ent : systemUI->emitterEntries) {
							if (ent.presetName == presetName) {
								exists = true;
								break;
							}
						}
						if (!exists) {
							NiagaraSystemUI::EmitterEntryUI newEntry;
							newEntry.presetName = presetName;
							newEntry.startTime = 0.0f;
							newEntry.duration = -1.0f;
							newEntry.autoPlay = true;
							systemUI->emitterEntries.push_back(newEntry);
						}
					}
					// 2 UI 側にあって ParticleSystem 側にない（削除された）ものを除外
					systemUI->emitterEntries.erase(
						std::remove_if(systemUI->emitterEntries.begin(),
							systemUI->emitterEntries.end(),
							[&](const NiagaraSystemUI::EmitterEntryUI& ent) {
								return std::find(presetNames->begin(),
									presetNames->end(),
									ent.presetName) == presetNames->end();
							}),
						systemUI->emitterEntries.end()
					);
				}
			}

			if (systemUI->emitterEntries.empty()) {
				ImGui::TextDisabled("プリセットが登録されていません。\nNE_* カードを System に登録してください。");
			}
			else {
				// 各 Emitter ごとに時間設定 UI を表示
				for (size_t idx = 0; idx < systemUI->emitterEntries.size(); ++idx) {
					auto& uiEntry = systemUI->emitterEntries[idx];

					// プリセットごとに ImGui ID を分ける（同じラベルが複数並ぶため）
					ImGui::PushID(static_cast<int>(idx));

					// プリセット名表示（読み取り専用）
					ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f),
						"[%zu] %s", idx, uiEntry.presetName.c_str());

					// startTime
					ImGui::DragFloat("Start Time (sec)", &uiEntry.startTime,
						0.01f, 0.0f, 60.0f, "%.2f");

					// duration（-1 で無制限）
					ImGui::DragFloat("Duration (sec, -1=無制限)", &uiEntry.duration,
						0.05f, -1.0f, 60.0f, "%.2f");

					// autoPlay
					ImGui::Checkbox("Auto Play", &uiEntry.autoPlay);

					ImGui::Separator();
					ImGui::PopID();
				}
			}

			ImGui::TextWrapped(
				"Start Time: System 再生からこの時間後に Emit 開始\n"
				"Duration: 発火開始から何秒で停止するか (-1で無制限)\n"
				"Save System JSON で設定が保存されます");

			ImGui::End();
			return;
		}

		// System も選ばれていない場合は従来通りメッセージ
		ImGui::TextColored(ImVec4(1, 1, 0, 1),
			"中央の NS_* または NE_* パネルをクリックして\n"
			"System / Emitter を選択してください。");
		ImGui::End();
		return;
	}


	if (!particle) {
		ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1),
			"ParticleManager がありません。");
		ImGui::End();
		return;
	}

	// モジュール名一覧（Canvas と対応）
	static const char* kModuleNames[] = {
		"Name",
		"Emitter Settings",
		"Emitter Spawn",
		"Emitter Update",
		"Particle Spawn",
		"Particle Update",
		"Render"
	};

	int moduleIndex = emitterUI->selectedModuleIndex;
	if (moduleIndex < 0) moduleIndex = 0;
	if (moduleIndex > 6) moduleIndex = 6;

	ImGui::Text("エミッタ : %s", emitterUI->name.c_str());
	ImGui::Text("選択中モジュール : %s", kModuleNames[moduleIndex]);
	ImGui::Separator();

	// 右側パネルでも Niagara Editor の ON/OFF を操作
	ImGui::Checkbox("Niagara Editor を表示", &showNiagaraUI_);
	ImGui::Separator();

	// ---- モジュール 0: Name（エミッタ名＆プリセット名＆System紐付け） ----
	if (moduleIndex == 0)
	{
		// =====================================================
		// 1) エミッタ名
		// =====================================================
		char emitterNameBuf[64]{};
		strncpy_s(emitterNameBuf, sizeof(emitterNameBuf),
			emitterUI->name.c_str(), _TRUNCATE);

		// 旧名を覚えておく（プリセット名と同じなら一緒に変更するため）
		std::string oldEmitterName = emitterUI->name;

		if (ImGui::InputText("エミッタ名", emitterNameBuf, sizeof(emitterNameBuf))) {
			emitterUI->name = emitterNameBuf;

			// 「プリセット名が旧エミッタ名と同じ」なら、一緒にリネーム
			if (emitterUI->presetName == oldEmitterName) {
				emitterUI->presetName = emitterUI->name;
				emitPresetName = emitterUI->presetName;
			}
		}

		// =====================================================
		// 2) プリセット名
		// =====================================================
		char presetNameBuf[64]{};
		strncpy_s(presetNameBuf, sizeof(presetNameBuf),
			emitterUI->presetName.c_str(), _TRUNCATE);
		if (ImGui::InputText("プリセット名", presetNameBuf, sizeof(presetNameBuf))) {
			emitterUI->presetName = presetNameBuf;
		}

		// エディタ全体で使うプリセット名にも反映
		emitPresetName = emitterUI->presetName;

		ImGui::TextWrapped(
			"同じプリセット名を持つエミッタは、同じパーティクル設定を共有します。\n"
			"（エミッタ名とプリセット名を同じにしておくと分かりやすいです）");

		ImGui::Separator();

		// =====================================================
		// 3) System への登録 UI
		// =====================================================
		ImGui::SeparatorText("System");

		ParticleManager* pm = particle.get();

		// --- System 名 入力欄（Emitter 固有） ---
		char systemBuf[64]{};
		std::snprintf(systemBuf, sizeof(systemBuf), "%s", emitterUI->systemName.c_str());
		if (ImGui::InputText("System名 (新規/既存)", systemBuf, IM_ARRAYSIZE(systemBuf))) {
			emitterUI->systemName = systemBuf;
			// シーン共通の入力値にも反映しておく（任意）
			systemNameInput_ = emitterUI->systemName;
		}

		// 既存System名一覧を取得
		std::vector<std::string> systemNames;
		if (pm) {
			systemNames = pm->GetAllSystemNames();
		}

		// 既存Systemをコンボで選択
		if (!systemNames.empty()) {
			// 初期インデックスの決定
			if (systemComboIndex_ < 0 ||
				systemComboIndex_ >= static_cast<int>(systemNames.size())) {

				systemComboIndex_ = 0;

				// Emitter にすでに systemName があれば、そのインデックスを探す
				if (!emitterUI->systemName.empty()) {
					for (int i = 0; i < static_cast<int>(systemNames.size()); ++i) {
						if (systemNames[i] == emitterUI->systemName) {
							systemComboIndex_ = i;
							break;
						}
					}
				}
			}

			const char* preview = systemNames[systemComboIndex_].c_str();

			if (ImGui::BeginCombo("既存 System", preview)) {
				for (int i = 0; i < static_cast<int>(systemNames.size()); ++i) {
					bool isSelected = (i == systemComboIndex_);
					if (ImGui::Selectable(systemNames[i].c_str(), isSelected)) {
						systemComboIndex_ = i;
						emitterUI->systemName = systemNames[i];   // Emitter に反映
						systemNameInput_ = emitterUI->systemName; // 任意で共通入力にも反映
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		else {
			ImGui::TextDisabled("登録済み System はありません");
		}

		if (ImGui::Button("このプリセットを System に追加")) {
			if (pm && !emitterUI->presetName.empty()) {

				// 優先順位: Emitter の systemName > コンボで選んだ名前
				std::string sysName = emitterUI->systemName;

				if (sysName.empty() &&
					systemComboIndex_ >= 0 &&
					systemComboIndex_ < static_cast<int>(systemNames.size())) {
					sysName = systemNames[systemComboIndex_];
				}

				if (!sysName.empty()) {
					pm->RegisterSystemPreset(sysName, emitterUI->presetName);

					// Emitter 側の systemName も確定させる
					emitterUI->systemName = sysName;
					systemNameInput_ = sysName;
				}
			}
		}

		// =====================================================
		// 4) System 単位で Emit する UI
		// =====================================================
		ImGui::SeparatorText("System Emit");

		if (!systemNames.empty()) {
			if (emitSystemComboIndex_ < 0 ||
				emitSystemComboIndex_ >= static_cast<int>(systemNames.size())) {
				emitSystemComboIndex_ = 0;
			}

			const char* emitPreview = systemNames[emitSystemComboIndex_].c_str();

			if (ImGui::BeginCombo("再生する System", emitPreview)) {
				for (int i = 0; i < static_cast<int>(systemNames.size()); ++i) {
					bool isSelected = (i == emitSystemComboIndex_);
					if (ImGui::Selectable(systemNames[i].c_str(), isSelected)) {
						emitSystemComboIndex_ = i;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::Button("System を Emit")) {
				auto* pm2 = particle.get();
				if (pm2 &&
					emitSystemComboIndex_ >= 0 &&
					emitSystemComboIndex_ < static_cast<int>(systemNames.size())) {

					const std::string& sysName = systemNames[emitSystemComboIndex_];

					// 位置は現在の emitterTransform を使って Emit
					pm2->EmitSystemByName(sysName, emitterTransform);
				}
			}
		}
		else {
			ImGui::TextDisabled("Emit できる System がありません");
		}

		ImGui::End();
		return;
	}


	// それ以外のモジュールはプリセットを参照
	ParticlePreset* preset = particle->FindPreset(emitterUI->presetName);

	// ★ プリセットがまだ存在しない場合は、その場でデフォルトを新規作成して保存
	if (!preset && !emitterUI->presetName.empty()) {
		ParticlePreset newPreset{};
		newPreset.name = emitterUI->presetName;

		// デフォルト値のまま JSON 保存 → 内部の presets_ にも登録される
		particle->SavePresetToJson(newPreset);

		// 作成し直したものを取得
		preset = particle->FindPreset(emitterUI->presetName);
	}

	if (!preset) {
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
			"プリセット \"%s\" を作成できませんでした。",
			emitterUI->presetName.c_str());
		ImGui::End();
		return;
	}

	// ---- 各モジュールごとの UI ----
	switch (moduleIndex)
	{
	case 1: // Emitter Settings
	{
		ImGui::SeparatorText("Emitter Settings");

		const char* vertexItems[] = { "Plane", "Ring", "Cylinder" };
		int v = static_cast<int>(preset->emitterSettings.vertexType);
		if (ImGui::Combo("頂点タイプ", &v, vertexItems, IM_ARRAYSIZE(vertexItems))) {
			preset->emitterSettings.vertexType =
				static_cast<VertexDataType>(v);
		}

		const char* blendItems[] = {
			"None","Normal","Add","Subtract","Multiply","Screen"
		};
		int b = static_cast<int>(preset->emitterSettings.blendMode);
		if (ImGui::Combo("ブレンドモード", &b, blendItems, IM_ARRAYSIZE(blendItems))) {
			preset->emitterSettings.blendMode =
				static_cast<BlendMode>(b);
		}
	}
	break;

	case 2: // Emitter Spawn
	{
		ImGui::SeparatorText("Emitter Spawn");

		int spawnCount = static_cast<int>(preset->emitterSpawn.count);
		if (ImGui::DragInt("発生数", &spawnCount, 1, 1, 10000)) {
			if (spawnCount < 1)      spawnCount = 1;
			if (spawnCount > 10000)  spawnCount = 10000;
			preset->emitterSpawn.count = static_cast<uint32_t>(spawnCount);
		}

		ImGui::DragFloat("発生間隔(秒)",
			&preset->emitterSpawn.frequency,
			0.01f, 0.0f, 60.0f);
		ImGui::Checkbox("ループ再生", &preset->emitterSpawn.repeat);
		ImGui::Checkbox("ランダム位置", &preset->emitterSpawn.useRandomPosition);
	}
	break;

	case 3: // Emitter Update
	{
		ImGui::SeparatorText("Emitter Update");

		ImGui::Text("エミッタ Transform（プレビュー用）");
		ImGui::DragFloat3("位置", &emitterTransform.translate.x, 0.1f);
		ImGui::DragFloat3("回転", &emitterTransform.rotate.x, 0.1f);
		ImGui::DragFloat3("スケール", &emitterTransform.scale.x, 0.1f, 0.0f, 100.0f);
	}
	break;

	case 4: // Particle Spawn
	{
		ImGui::SeparatorText("Particle Spawn");

		// --- 初期スケール ---
		ImGui::DragFloat3("初期スケール",
			&preset->particleSpawn.initialScale.x,
			0.01f, 0.0f, 100.0f);

		ImGui::Checkbox("初期スケールをランダムにする",
			&preset->particleSpawn.initialScaleRandom.useRandom);
		ImGui::DragFloat3("Scale Min",
			&preset->particleSpawn.initialScaleRandom.minValue.x,
			0.01f, -1000.0f, 1000.0f);
		ImGui::DragFloat3("Scale Max",
			&preset->particleSpawn.initialScaleRandom.maxValue.x,
			0.01f, -1000.0f, 1000.0f);

		ImGui::Separator();

		// --- 初期回転 ---
		ImGui::DragFloat3("初期回転",
			&preset->particleSpawn.initialRotate.x,
			0.01f, -6.28f, 6.28f);

		ImGui::Checkbox("初期回転をランダムにする",
			&preset->particleSpawn.initialRotateRandom.useRandom);
		ImGui::DragFloat3("Rot Min",
			&preset->particleSpawn.initialRotateRandom.minValue.x,
			0.01f, -6.28f, 6.28f);
		ImGui::DragFloat3("Rot Max",
			&preset->particleSpawn.initialRotateRandom.maxValue.x,
			0.01f, -6.28f, 6.28f);

		ImGui::Separator();

		// --- 初期オフセット ---
		ImGui::DragFloat3("初期オフセット",
			&preset->particleSpawn.initialOffset.x,
			0.01f, -1000.0f, 1000.0f);

		ImGui::Checkbox("初期オフセットをランダムにする",
			&preset->particleSpawn.initialOffsetRandom.useRandom);
		ImGui::DragFloat3("Offset Min",
			&preset->particleSpawn.initialOffsetRandom.minValue.x,
			0.01f, -1000.0f, 1000.0f);
		ImGui::DragFloat3("Offset Max",
			&preset->particleSpawn.initialOffsetRandom.maxValue.x,
			0.01f, -1000.0f, 1000.0f);
	}
	break;


	case 5: // Particle Update
	{
		ImGui::SeparatorText("Particle Update");

		// --- 速度 ---
		ImGui::DragFloat3("速度",
			&preset->particleUpdate.velocity.x,
			0.01f, -1000.0f, 1000.0f);

		ImGui::Checkbox("速度をランダムにする",
			&preset->particleUpdate.velocityRandom.useRandom);
		ImGui::DragFloat3("Vel Min",
			&preset->particleUpdate.velocityRandom.minValue.x,
			0.01f, -1000.0f, 1000.0f);
		ImGui::DragFloat3("Vel Max",
			&preset->particleUpdate.velocityRandom.maxValue.x,
			0.01f, -1000.0f, 1000.0f);

		ImGui::Separator();

		// --- 回転速度 ---
		ImGui::DragFloat3("回転速度",
			&preset->particleUpdate.rotationSpeed.x,
			0.01f, -10.0f, 10.0f);

		ImGui::Checkbox("回転速度をランダムにする",
			&preset->particleUpdate.rotationRandom.useRandom);
		ImGui::DragFloat3("RotSpd Min",
			&preset->particleUpdate.rotationRandom.minValue.x,
			0.01f, -10.0f, 10.0f);
		ImGui::DragFloat3("RotSpd Max",
			&preset->particleUpdate.rotationRandom.maxValue.x,
			0.01f, -10.0f, 10.0f);

		ImGui::Separator();

		// --- スケール速度 ---
		ImGui::DragFloat3("スケール速度",
			&preset->particleUpdate.scaleSpeed.x,
			0.01f, -10.0f, 10.0f);

		ImGui::Checkbox("スケール速度をランダムにする",
			&preset->particleUpdate.scaleRandom.useRandom);
		ImGui::DragFloat3("ScaleSpd Min",
			&preset->particleUpdate.scaleRandom.minValue.x,
			0.01f, -10.0f, 10.0f);
		ImGui::DragFloat3("ScaleSpd Max",
			&preset->particleUpdate.scaleRandom.maxValue.x,
			0.01f, -10.0f, 10.0f);

		ImGui::Separator();

		ImGui::Checkbox("重力を使う",
			&preset->particleUpdate.useGravity);
		ImGui::DragFloat("寿命(秒)",
			&preset->particleUpdate.lifeTime,
			0.01f, 0.0f, 100.0f);

		// ここから：下パネルのカーブエディタを開く
		if (ImGui::Button("スケールをカーブで編集...")) {
			curveEditorMode_ = CurveEditorMode::Scale;
			curveEditorTarget_ = &preset->particleUpdate.scaleCurve;
			curveEditorTitle_ = "Scale Over Life";
		}
	}
	break;


	case 6: // Render
	{
		ImGui::SeparatorText("Render");

		// ベース色
		ImGui::ColorEdit4("ベース色 (RGBA)", &preset->render.color.x);

		//========================
		// カラーグラデーション
		//========================
		ImGui::SeparatorText("カラーグラデーション");
		ImGui::Checkbox("グラデーションを使う", &preset->render.colorGradient.enabled);

		auto& grad = preset->render.colorGradient;

		// シンプルに「0 と 1 の 2 キー」を最低保証
		if (grad.keys.size() < 2) {
			grad.keys.resize(2);
			grad.keys[0].t = 0.0f;
			grad.keys[1].t = 1.0f;
			grad.keys[0].color = preset->render.color;
			grad.keys[1].color = preset->render.color;
		}

		// 開始色・終了色を直接編集（本体を触る）
		ImGui::ColorEdit4("開始色", &grad.keys[0].color.x);
		ImGui::ColorEdit4("終了色", &grad.keys[1].color.x);

		if (ImGui::Button("グラデーションをリセット")) {
			grad.keys[0].t = 0.0f;
			grad.keys[1].t = 1.0f;
			grad.keys[0].color = preset->render.color;
			grad.keys[1].color = preset->render.color;
		}

		//========================
		// 時間カーブ
		//========================
		ImGui::SeparatorText("時間カーブ");

		if (ImGui::Button("グラデーション時間をカーブで編集...")) {
			curveEditorMode_ = CurveEditorMode::GradientTime;
			curveEditorTarget_ = &preset->render.gradientTimeCurve;
			curveEditorTitle_ = "Gradient Time (Age -> GradT)";
		}

		//========================
		// カラー強度/α カーブ
		//========================
		if (ImGui::Button("カラー強度/αをカーブで編集...")) {
			curveEditorMode_ = CurveEditorMode::Color;
			curveEditorTarget_ = &preset->render.colorCurve;
			curveEditorTitle_ = "Color Intensity / Alpha Over Life";
		}

		ImGui::Checkbox("ビルボード", &preset->render.useBillboard);
		ImGui::Checkbox("上下反転(Flip Y)", &preset->render.flipY);



		ImGui::SeparatorText("テクスチャ");

		// --- Resources/ParticleTexture からファイル一覧を取得 ---
		static bool sTextureListInitialized = false;
		static std::vector<std::string> sTextureList;

		if (!sTextureListInitialized) {
			namespace fs = std::filesystem;
			sTextureList.clear();

			const std::string dir = "Resources/ParticleTexture";
			if (fs::exists(dir)) {
				for (auto& entry : fs::directory_iterator(dir)) {
					if (!entry.is_regular_file()) { continue; }

					std::string ext = entry.path().extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					// 対象にする拡張子
					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
						sTextureList.push_back(entry.path().filename().string());
					}
				}
				std::sort(sTextureList.begin(), sTextureList.end());
			}

			sTextureListInitialized = true;
		}

		if (!sTextureList.empty()) {

			// 現在のプリセットのパスから、リスト内のインデックスを求める
			int currentTextureIndex = -1;
			std::string currentName;

			if (!preset->emitterSettings.textureFilePath.empty()) {
				const std::string& path = preset->emitterSettings.textureFilePath;
				const std::string prefix = "Resources/ParticleTexture/";

				if (path.rfind(prefix, 0) == 0) {
					currentName = path.substr(prefix.size());
				}
				else {
					// 古いデータなどでフルパスでない場合も一応対応
					currentName = path;
				}
			}

			for (int i = 0; i < static_cast<int>(sTextureList.size()); ++i) {
				if (sTextureList[i] == currentName) {
					currentTextureIndex = i;
					break;
				}
			}

			// 見つからなかった／未設定なら 0 番をデフォルトに
			if (currentTextureIndex < 0) {
				currentTextureIndex = 0;
				preset->emitterSettings.textureFilePath =
					"Resources/ParticleTexture/" + sTextureList[0];
			}

			const char* previewText = sTextureList[currentTextureIndex].c_str();

			if (ImGui::BeginCombo("テクスチャファイル", previewText)) {

				// SRV マネージャ（プレビュー表示に使用）
				SrvManager* srvManager = SrvManager::GetInstance();

				for (int i = 0; i < static_cast<int>(sTextureList.size()); ++i) {
					bool isSelected = (i == currentTextureIndex);
					const std::string& fileName = sTextureList[i];

					// 選択
					if (ImGui::Selectable(fileName.c_str(), isSelected)) {
						currentTextureIndex = i;
						preset->emitterSettings.textureFilePath =
							"Resources/ParticleTexture/" + fileName;
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}

					// マウスが乗ったらプレビュー画像を表示
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						{
							std::string fullPath = "Resources/ParticleTexture/" + fileName;

							// テクスチャ読み込み（キャッシュされていれば再利用される）
							TextureManager::GetInstance()->LoadTexture(fullPath);
							uint32_t texIndex =
								TextureManager::GetInstance()->GetTextureIndexByFilePath(fullPath);

							// SRV の GPU ハンドルを取得
							D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
								srvManager->GetGPUDescriptorHandle(texIndex);

							// ImGui::Image に渡す ID（DX12 なので ptr をそのまま渡す）
							ImTextureID imguiTexId = (ImTextureID)gpuHandle.ptr;

							ImVec2 previewSize(256.0f, 256.0f);
							ImGui::Image(imguiTexId, previewSize);
						}
						ImGui::EndTooltip();
					}
				}

				ImGui::EndCombo();
			}
		}
		else {
			ImGui::TextColored(
				ImVec4(1, 0, 0, 1),
				"Resources/ParticleTexture 以下にテクスチャが見つかりません");
		}
	}
	break;
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::Button("プリセットを保存 (Niagara Inspector)")) {
		particle->SavePresetToJson(*preset);
	}

	ImGui::Separator();

	// ここから新EmitterInstanceルート
	if (ImGui::Button("Emit 実行")) {
		if (particle && !emitPresetName.empty()) {

			// 新しい EmitterInstance を作成して emitterInstances_ に登録
			ParticleEmitterInstance* inst =
				particle->CreateEmitterInstanceFromPreset(emitPresetName, emitterTransform);

			// 失敗時だけ簡易ログ（任意）
			if (!inst) {
				// ここで ImGui::Text で警告を出したり、Logger::Log してもOK
				// Logger::Log("Emit 実行: Preset が見つかりません: " + emitPresetName);
			}
		}
	}



	ImGui::SameLine();
	if (ImGui::Button("全プリセット再読み込み")) {
		if (particle) {
			particle->LoadAllPresets();
		}
	}


	ImGui::End();
}

//--------------------------------------------------
// 下：Camera Control
//--------------------------------------------------
void ParticleEditorScene::DrawCameraControlPanel(const ImVec2& pos, const ImVec2& size, int panelFlags)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGuiWindowFlags flags = static_cast<ImGuiWindowFlags>(panelFlags);

	if (!ImGui::Begin(kWindowName_Camera, nullptr, flags))
	{
		ImGui::End();
		return;
	}

	if (camera) {
		ImGui::Text("Control");
		ImGui::Separator();

		// Niagara Editor のトグル
		ImGui::Checkbox("Niagara Editor を表示", &showNiagaraUI_);
		ImGui::Separator();

		// 既存のデバッグカメラ ON/OFF
		ImGui::Checkbox("Enable Debug Camera", &debugCameraEnabled_);

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

		Vector3 camPos = camera->GetTranslate();
		Vector3 camRot = camera->GetRotate();

		if (ImGui::DragFloat3("Camera Pos", &camPos.x, 0.1f)) {
			camera->SetTranslate(camPos);
		}
		if (ImGui::DragFloat3("Camera Rot", &camRot.x, 0.01f)) {
			camera->SetRotate(camRot);
		}
	}

	ImGui::End();
}

//--------------------------------------------------
// 下：Curve Editor
//--------------------------------------------------


void ParticleEditorScene::DrawCurveEditorPanel(const ImVec2& pos, const ImVec2& size, int panelFlags)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGuiWindowFlags flags = static_cast<ImGuiWindowFlags>(panelFlags)
		| ImGuiWindowFlags_NoScrollWithMouse; // 横スクロールは許可（サイズが少し狭くなっても操作しやすく）

	if (!ImGui::Begin("Curve Editor", nullptr, flags))
	{
		ImGui::End();
		return;
	}

	if (!curveEditorTarget_) {
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
			"編集対象のカーブがありません。");
		if (ImGui::Button("Camera Control に戻る")) {
			curveEditorMode_ = CurveEditorMode::None;
		}
		ImGui::End();
		return;
	}

	bool open = (curveEditorMode_ != CurveEditorMode::None);

	DrawCurve1DEditor(curveEditorTitle_.c_str(), *curveEditorTarget_, &open);

	if (!open) {
		curveEditorMode_ = CurveEditorMode::None;
		curveEditorTarget_ = nullptr;
	}

	ImGui::End();
}

#endif // USE_IMGUI
#include "EditorLayout.h"

#ifdef USE_IMGUI

#include <externals/imgui/imgui_internal.h>
#include <algorithm>
#include <cstdio>

#include "SrvManager.h"
#include "PostEffectManager.h"
#include "SceneManager.h"
#include "AbstractSceneFactory.h"
#include "BaseScene.h"

// ================================================================
//  初期化 / 終了
// ================================================================

void EditorLayout::Initialize()
{
	ApplyStyle();

	dockLayoutBuilt_ = false;
	requestResetLayout_ = false;
	requestedSceneName_.clear();
}

void EditorLayout::Finalize()
{
}

// ================================================================
//  メインループから呼ばれる BeginFrame / EndFrame
// ================================================================

void EditorLayout::BeginFrame()
{
	if (!enabled_) {
		return;
	}

	// 1. DockSpaceを構築
	BuildDockSpace();

	// 2. 各パネルを描画
	if (showViewport_)       DrawViewportPanel();
	if (showActorsPalette_)  DrawActorsPalettePanel();
	if (showContentBrowser_) DrawContentBrowserPanel();
	if (showOutliner_)       DrawOutlinerPanel();
	if (showDetails_)        DrawDetailsPanel();
}

void EditorLayout::EndFrame()
{
	if (!enabled_) {
		return;
	}

	// シーン切り替え予約があればここで実行
	if (!requestedSceneName_.empty()) {
		SceneManager::GetInstance()->ChangeScene(requestedSceneName_);
		requestedSceneName_.clear();
	}
}

// ================================================================
//  DockSpace（フルスクリーンの親ウィンドウ + MenuBar）
// ================================================================

void EditorLayout::BuildDockSpace()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin(kDockSpaceName, nullptr, window_flags);
	ImGui::PopStyleVar(3);

	DrawMenuBar();

	ImGuiID dockspace_id = ImGui::GetID(kDockSpaceName);

	// 初回 or リセット要求時にレイアウトを構築
	if (!dockLayoutBuilt_ || requestResetLayout_) {
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		// ================================================================
		//  UE5風 5分割レイアウト
		//
		//  ┌────────┬──────────────────────┬──────────────┐
		//  │        │                      │              │
		//  │        │       Viewport       │   Outliner   │
		//  │ Actors │                      │              │
		//  │ Palette├──────────────────────┼──────────────┤
		//  │        │                      │              │
		//  │        │   Content Browser    │   Details    │
		//  │        │                      │              │
		//  └────────┴──────────────────────┴──────────────┘
		//
		//  全体比率: 左 15% : 中央 63% : 右 22%
		//  中央の縦分割: Viewport 70% : Content Browser 30%
		//  右の縦分割: Outliner 40% : Details 60%
		// ================================================================

		ImGuiID dock_main = dockspace_id;

		// ① 左端から 15% を切り出す → Actors Palette
		ImGuiID dock_left = ImGui::DockBuilderSplitNode(
			dock_main, ImGuiDir_Left, 0.15f, nullptr, &dock_main);

		// ② 残り(85%)の右端から 22% を切り出す
		//    比率補正: 0.22f / 0.85f ≒ 0.259f
		ImGuiID dock_right = ImGui::DockBuilderSplitNode(
			dock_main, ImGuiDir_Right, 0.22f / 0.85f, nullptr, &dock_main);

		// ③ 中央(dock_main)を上下に分割: Content Browser を下30%に
		ImGuiID dock_center_bottom = ImGui::DockBuilderSplitNode(
			dock_main, ImGuiDir_Down, 0.30f, nullptr, &dock_main);
		//   dock_main が Viewport(上70%)、dock_center_bottom が Content Browser

		// ④ 右側を上下に分割: Outliner 40% / Details 60%
		ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(
			dock_right, ImGuiDir_Down, 0.60f, nullptr, &dock_right);
		//   dock_right が Outliner(上40%)、dock_right_bottom が Details(下60%)

		// --- 各ウィンドウをドッキング ---
		ImGui::DockBuilderDockWindow(kActorsPaletteName, dock_left);
		ImGui::DockBuilderDockWindow(kViewportName, dock_main);
		ImGui::DockBuilderDockWindow(kContentBrowserName, dock_center_bottom);
		ImGui::DockBuilderDockWindow(kOutlinerName, dock_right);
		ImGui::DockBuilderDockWindow(kDetailsName, dock_right_bottom);

		ImGui::DockBuilderFinish(dockspace_id);

		dockLayoutBuilt_ = true;
		requestResetLayout_ = false;
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

// ================================================================
//  メニューバー
// ================================================================

void EditorLayout::DrawMenuBar()
{
	if (!ImGui::BeginMenuBar()) {
		return;
	}

	// File
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Exit", "Alt+F4")) {
			// 終了処理は将来MyGame側で
		}
		ImGui::EndMenu();
	}

	// Scene
	DrawSceneMenu();

	// Window
	if (ImGui::BeginMenu("Window")) {
		ImGui::MenuItem(kViewportName, nullptr, &showViewport_);
		ImGui::MenuItem(kActorsPaletteName, nullptr, &showActorsPalette_);
		ImGui::MenuItem(kContentBrowserName, nullptr, &showContentBrowser_);
		ImGui::MenuItem(kOutlinerName, nullptr, &showOutliner_);
		ImGui::MenuItem(kDetailsName, nullptr, &showDetails_);
		ImGui::Separator();
		ImGui::MenuItem("Stat FPS", nullptr, &showStatFPS_);
		ImGui::EndMenu();
	}

	// Layout
	if (ImGui::BeginMenu("Layout")) {
		if (ImGui::MenuItem("Reset Layout")) {
			requestResetLayout_ = true;
		}
		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
}

// ================================================================
//  Scene メニュー
// ================================================================

void EditorLayout::DrawSceneMenu()
{
	if (!ImGui::BeginMenu("Scene")) {
		return;
	}

	SceneManager* sm = SceneManager::GetInstance();

	bool transitioning = sm->IsTransitioning();
	if (transitioning) {
		ImGui::TextDisabled("(transitioning...)");
		ImGui::Separator();
	}

	AbstractSceneFactory* factory = sm->GetSceneFactory();
	if (factory) {
		const std::vector<std::string> names = factory->GetSceneNameList();
		for (const std::string& name : names) {
			if (ImGui::MenuItem(name.c_str(), nullptr, false, !transitioning)) {
				requestedSceneName_ = name;
			}
		}
	}
	else {
		ImGui::TextDisabled("(SceneFactory not set)");
	}

	ImGui::EndMenu();
}

// ================================================================
//  Viewport (中央上)
// ================================================================

void EditorLayout::DrawViewportPanel()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	if (ImGui::Begin(kViewportName, &showViewport_)) {
		ImTextureID textureID = (ImTextureID)SrvManager::GetInstance()
			->GetGPUDescriptorHandle(PostEffectManager::GetInstance()->GetSrvIndex()).ptr;

		ImVec2 avail = ImGui::GetContentRegionAvail();

		if (avail.x > 0.0f && avail.y > 0.0f) {
			ImGui::Image(textureID, avail);
		}

		// ★ Stat FPS オーバーレイ (Viewportが表示されているときだけ、Viewport内に重ねる)
		if (showStatFPS_) {
			DrawStatFPSOverlay();
		}
	}
	ImGui::End();

	ImGui::PopStyleVar();
}

// ================================================================
//  Actors Palette (左: アクタを配置するパネル)
// ================================================================

void EditorLayout::DrawActorsPalettePanel()
{
	if (ImGui::Begin(kActorsPaletteName, &showActorsPalette_)) {
		ImGui::TextDisabled("Place Actors");
		ImGui::Separator();

		// TODO: 基本カテゴリとアクタ一覧(後のステップで実装)
		// UE5では「基本」「ライト」「シェイプ」「ボリューム」などのカテゴリと、
		// そこに配置可能なアクタのアイコン一覧が並ぶ
		ImGui::TextUnformatted("(ここに配置可能な");
		ImGui::TextUnformatted(" アクタ一覧が入ります)");
		ImGui::Spacing();

		// プレースホルダとしてカテゴリだけ表示
		// ※ u8"..." は C++20 以降 char8_t になるため、ImGui (const char*) と型が合わない
		//   ソース自体を UTF-8 with BOM で保存していれば、"..." で日本語を使って問題ない
		if (ImGui::CollapsingHeader("基本", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent();
			ImGui::TextDisabled("アクタ");
			ImGui::TextDisabled("キャラクター");
			ImGui::TextDisabled("ポーン");
			ImGui::TextDisabled("ポイントライト");
			ImGui::Unindent();
		}
		if (ImGui::CollapsingHeader("ライト")) {
			ImGui::Indent();
			ImGui::TextDisabled("ディレクショナルライト");
			ImGui::TextDisabled("ポイントライト");
			ImGui::TextDisabled("スポットライト");
			ImGui::Unindent();
		}
		if (ImGui::CollapsingHeader("シェイプ")) {
			ImGui::Indent();
			ImGui::TextDisabled("キューブ");
			ImGui::TextDisabled("スフィア");
			ImGui::TextDisabled("プレーン");
			ImGui::Unindent();
		}
	}
	ImGui::End();
}

// ================================================================
//  Content Browser (中央下)
// ================================================================

void EditorLayout::DrawContentBrowserPanel()
{
	if (ImGui::Begin(kContentBrowserName, &showContentBrowser_)) {
		ImGui::TextDisabled("Assets");
		ImGui::Separator();
		ImGui::TextUnformatted("(ここにアセット一覧が入ります)");
	}
	ImGui::End();
}

// ================================================================
//  Outliner (右上)
// ================================================================

void EditorLayout::DrawOutlinerPanel()
{
	if (ImGui::Begin(kOutlinerName, &showOutliner_)) {
		ImGui::TextDisabled("Scene Objects");
		ImGui::Separator();
		ImGui::TextUnformatted("(ここにシーンのオブジェクト一覧が入ります)");
	}
	ImGui::End();
}

// ================================================================
//  Details / Inspector (右下)
// ================================================================

void EditorLayout::DrawDetailsPanel()
{
	if (ImGui::Begin(kDetailsName, &showDetails_)) {
		ImGui::TextDisabled("Selected Object");
		ImGui::Separator();
		ImGui::TextUnformatted("(ここに選択オブジェクトの詳細が入ります)");

		// デバッグ用に現在シーン情報を表示
		BaseScene* current = SceneManager::GetInstance()->GetCurrentScene();
		ImGui::Separator();
		ImGui::Text("Current Scene ptr: %p", current);
	}
	ImGui::End();
}

// ================================================================
//  Stat FPS オーバーレイ (Viewport左上に半透明で重ねる)
// ================================================================

void EditorLayout::DrawStatFPSOverlay()
{
	// ----- 現在のウィンドウ(Viewport)上に直接描画するため、DrawListを取得 -----
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// ウィンドウの左上スクリーン座標を取得
	ImVec2 viewportMin = ImGui::GetWindowPos();

	// Viewportのタブバーの高さ分だけ下げたい場合に使用する余白
	// (PushStyleVarでpadding=0にしているので、ContentRegionの座標からmarginだけ足す)
	const float marginX = 8.0f;
	const float marginY = 8.0f;

	// ContentRegionの開始位置を基準にする
	ImVec2 cr_min = ImGui::GetWindowContentRegionMin();
	ImVec2 overlayPos = ImVec2(
		viewportMin.x + cr_min.x + marginX,
		viewportMin.y + cr_min.y + marginY
	);

	// ----- テキスト内容を組み立て -----
	char line1[64];
	char line2[64];
	char line3[64];
	snprintf(line1, sizeof(line1), "FPS          : %6.2f", stat_fps_);
	snprintf(line2, sizeof(line2), "Frame        : %6.2f ms", stat_frameTimeMs_);
	snprintf(line3, sizeof(line3), "Average FPS  : %6.2f", stat_averageFps_);

	// ----- 背景矩形のサイズ計算 -----
	ImVec2 size1 = ImGui::CalcTextSize(line1);
	ImVec2 size2 = ImGui::CalcTextSize(line2);
	ImVec2 size3 = ImGui::CalcTextSize(line3);
	float maxTextW = (std::max)((std::max)(size1.x, size2.x), size3.x);
	float lineH = size1.y;

	const float padX = 10.0f;
	const float padY = 6.0f;

	ImVec2 rectMin = overlayPos;
	ImVec2 rectMax = ImVec2(
		rectMin.x + maxTextW + padX * 2.0f,
		rectMin.y + lineH * 3.0f + padY * 2.0f
	);

	// ----- 半透明黒の背景 -----
	ImU32 bgColor = IM_COL32(0, 0, 0, 150);  // 半透明黒
	drawList->AddRectFilled(rectMin, rectMax, bgColor, 4.0f);

	// ----- うっすらとした枠線 -----
	ImU32 borderColor = IM_COL32(255, 255, 255, 40);
	drawList->AddRect(rectMin, rectMax, borderColor, 4.0f, 0, 1.0f);

	// ----- テキストの描画 (UE5 Stat FPS風に黄色) -----
	// FPSの値によって色を変える(60以上は緑、30以上は黄色、それ以下は赤)
	ImU32 fpsColor;
	if (stat_fps_ >= 60.0f)      fpsColor = IM_COL32(0, 255, 0, 255);      // 緑
	else if (stat_fps_ >= 30.0f) fpsColor = IM_COL32(255, 255, 0, 255);    // 黄
	else                         fpsColor = IM_COL32(255, 80, 80, 255);    // 赤

	ImU32 labelColor = IM_COL32(230, 230, 230, 255);  // 白っぽい

	ImVec2 textPos = ImVec2(rectMin.x + padX, rectMin.y + padY);

	drawList->AddText(textPos, fpsColor, line1);
	textPos.y += lineH;
	drawList->AddText(textPos, labelColor, line2);
	textPos.y += lineH;
	drawList->AddText(textPos, labelColor, line3);
}

// ================================================================
//  ResetLayout
// ================================================================

void EditorLayout::ResetLayout()
{
	requestResetLayout_ = true;
}

// ================================================================
//  スタイル（UE5っぽい暗めのカラーリング）
// ================================================================

void EditorLayout::ApplyStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.WindowRounding = 4.0f;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.PopupRounding = 3.0f;

	style.WindowPadding = ImVec2(8, 8);
	style.FramePadding = ImVec2(6, 4);
	style.ItemSpacing = ImVec2(6, 4);
	style.ItemInnerSpacing = ImVec2(6, 4);
	style.IndentSpacing = 18.0f;

	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;

	colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.96f);
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);

	colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);

	colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.50f, 0.75f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.45f, 0.65f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.40f, 0.60f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);

	colors[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.50f, 0.75f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);

	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.50f, 0.75f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.50f, 0.75f, 1.00f);

	colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 0.90f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
}

#endif // USE_IMGUI
#include "EditorLayout.h"

#ifdef USE_IMGUI

#include <externals/imgui/imgui_internal.h>

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
	// スタイルを適用（UE5風の暗めなカラー）
	ApplyStyle();

	dockLayoutBuilt_ = false;
	requestResetLayout_ = false;
	requestedSceneName_.clear();
}

void EditorLayout::Finalize()
{
	// 今のところ特になし
}

// ================================================================
//  メインループから呼ばれる BeginFrame / EndFrame
// ================================================================

void EditorLayout::BeginFrame()
{
	if (!enabled_) {
		return;
	}

	// --- 1. DockSpaceを構築（フルスクリーンの親ウィンドウ） ---
	BuildDockSpace();

	// --- 2. 各パネルを描画 ---
	if (showViewport_)       DrawViewportPanel();
	if (showOutliner_)       DrawOutlinerPanel();
	if (showContentBrowser_) DrawContentBrowserPanel();
	if (showDetails_)        DrawDetailsPanel();
	if (showWorldSettings_)  DrawWorldSettingsPanel();
}

void EditorLayout::EndFrame()
{
	if (!enabled_) {
		return;
	}

	// --- シーン切り替え予約があれば、ここで実行 ---
	// （BeginFrame の最中に切り替えると、同フレーム内で scene_ が差し替わる前に
	//   パネルの描画が走ってしまうことがあるため、末尾で発行する）
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

	// メニューバー
	DrawMenuBar();

	// DockSpaceのIDを取得
	ImGuiID dockspace_id = ImGui::GetID(kDockSpaceName);

	// 初回 or リセット要求時にレイアウトを構築
	if (!dockLayoutBuilt_ || requestResetLayout_) {
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		ImGuiID dock_main = dockspace_id;

		// 左を20%切り出す (Outliner + Content Browser の領域)
		ImGuiID dock_left = ImGui::DockBuilderSplitNode(
			dock_main, ImGuiDir_Left, 0.20f, nullptr, &dock_main);

		// 右を25%切り出す (Details + World Settings の領域)
		ImGuiID dock_right = ImGui::DockBuilderSplitNode(
			dock_main, ImGuiDir_Right, 0.25f / (1.0f - 0.20f), nullptr, &dock_main);

		// 左側を上下に分割 (Outliner : Content Browser = 60 : 40)
		ImGuiID dock_left_bottom = ImGui::DockBuilderSplitNode(
			dock_left, ImGuiDir_Down, 0.40f, nullptr, &dock_left);

		// 右側を上下に分割 (Details : World Settings = 60 : 40)
		ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(
			dock_right, ImGuiDir_Down, 0.40f, nullptr, &dock_right);

		ImGui::DockBuilderDockWindow(kOutlinerName, dock_left);
		ImGui::DockBuilderDockWindow(kContentBrowserName, dock_left_bottom);
		ImGui::DockBuilderDockWindow(kViewportName, dock_main);
		ImGui::DockBuilderDockWindow(kDetailsName, dock_right);
		ImGui::DockBuilderDockWindow(kWorldSettingsName, dock_right_bottom);

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
			// 終了処理はMyGame側で行う想定
		}
		ImGui::EndMenu();
	}

	// Scene（シーン切り替え）★ここが今回の主役★
	DrawSceneMenu();

	// Window（各パネルの表示/非表示）
	if (ImGui::BeginMenu("Window")) {
		ImGui::MenuItem(kViewportName, nullptr, &showViewport_);
		ImGui::MenuItem(kOutlinerName, nullptr, &showOutliner_);
		ImGui::MenuItem(kContentBrowserName, nullptr, &showContentBrowser_);
		ImGui::MenuItem(kDetailsName, nullptr, &showDetails_);
		ImGui::MenuItem(kWorldSettingsName, nullptr, &showWorldSettings_);
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

	// --- SceneFactory からシーン名一覧を取得 ---
	// SceneManager 側に直接 factory_ への getter は無いが、
	// SetSceneFactory で渡している SceneFactory を取得する手段を今は持たない。
	// そこで、MyGame::Initialize で SceneManager に渡したものと同じ方法で
	// 直接 SceneFactory インスタンスを使いたい場合は getter を足してもよいが、
	// まずは素直に「現在のシーン」を表示しつつ、固定のシーン名リストを
	// AbstractSceneFactory 経由で取り出す。
	//
	// → ここでは SceneManager にシーンファクトリの参照がある前提で
	//    SceneManager::GetSceneFactory() を用意するのが筋だが、
	//    既存コードを極力壊さないため「SceneFactory の生ポインタ」を
	//    呼び出し側（MyGame）から EditorLayout に注入する手もある。
	//
	// ここでは SceneManager を仲介せずに「現在のシーン名は取得できない」
	// 前提でシンプルに実装する（切り替え先の予約のみ UI に出す）。

	// 遷移中は操作不可にする
	bool transitioning = sm->IsTransitioning();
	if (transitioning) {
		ImGui::TextDisabled("(transitioning...)");
		ImGui::Separator();
	}

	// --- シーン名リスト ---
	// 本来は SceneFactory を経由するべきだが、SceneManager に getter を
	// 追加していないのでここでは独立した方法で取得する必要がある。
	// ひとまず SceneManager に getter を追加する方向で実装（SceneManager.h を更新）。
	AbstractSceneFactory* factory = sm->GetSceneFactory();
	if (factory) {
		const std::vector<std::string> names = factory->GetSceneNameList();

		for (const std::string& name : names) {
			// 現在のシーン名との比較は難しい（BaseScene に名前フィールドが無い）ので
			// 選択マークなしのシンプルなメニューにする
			if (ImGui::MenuItem(name.c_str(), nullptr, false, !transitioning)) {
				// 予約のみ行い、実際の切り替えは EndFrame で実行
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
//  Viewport (中央)
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
	}
	ImGui::End();

	ImGui::PopStyleVar();
}

// ================================================================
//  Outliner (左上)
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
//  Content Browser (左下)
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
//  Details / Inspector (右上)
// ================================================================

void EditorLayout::DrawDetailsPanel()
{
	if (ImGui::Begin(kDetailsName, &showDetails_)) {
		ImGui::TextDisabled("Selected Object");
		ImGui::Separator();
		ImGui::TextUnformatted("(ここに選択オブジェクトの詳細が入ります)");
	}
	ImGui::End();
}

// ================================================================
//  World Settings (右下)
// ================================================================

void EditorLayout::DrawWorldSettingsPanel()
{
	if (ImGui::Begin(kWorldSettingsName, &showWorldSettings_)) {
		ImGui::TextDisabled("World / Camera");
		ImGui::Separator();
		ImGui::TextUnformatted("(ここにワールド/カメラの設定が入ります)");

		// 現在のシーン名をざっくり出したいならここに追加
		// （BaseScene に名前フィールドは無いので、参考情報として）
		BaseScene* current = SceneManager::GetInstance()->GetCurrentScene();
		ImGui::Separator();
		ImGui::Text("Current Scene ptr: %p", current);
	}
	ImGui::End();
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
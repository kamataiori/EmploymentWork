#include "BTNodeGraphEditor.h"

#include <imguiManager.h>
#ifdef USE_IMGUI
#include "externals/imgui/imnodes.h"
#endif
#include <json.hpp>

#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <cassert>

using namespace BTEditor;

#ifdef USE_IMGUI

//======================================================
// 色定数（ABGR 形式 for imnodes）
// imnodes は IM_COL32(R,G,B,A) マクロを使う
//======================================================
namespace {

	constexpr unsigned int kColorRoot = IM_COL32(60, 60, 160, 255);
	constexpr unsigned int kColorRootTitle = IM_COL32(40, 40, 120, 255);
	constexpr unsigned int kColorSeq = IM_COL32(50, 120, 50, 255);
	constexpr unsigned int kColorSeqTitle = IM_COL32(30, 90, 30, 255);
	constexpr unsigned int kColorSel = IM_COL32(150, 90, 20, 255);
	constexpr unsigned int kColorSelTitle = IM_COL32(110, 65, 10, 255);
	constexpr unsigned int kColorDec = IM_COL32(120, 50, 120, 255);
	constexpr unsigned int kColorDecTitle = IM_COL32(90, 30, 90, 255);
	constexpr unsigned int kColorLeaf = IM_COL32(30, 100, 140, 255);
	constexpr unsigned int kColorLeafTitle = IM_COL32(20, 70, 110, 255);

} // anonymous namespace

//======================================================
// 生成 / 破棄
//======================================================
BTNodeGraphEditor::BTNodeGraphEditor() = default;
BTNodeGraphEditor::~BTNodeGraphEditor() = default;

void BTNodeGraphEditor::Initialize()
{

	// スタイル調整
	// ※ NodePaddingHorizontal / NodePaddingVertical は
	//   imnodes の古いバージョンにのみ存在するため使用しない
	ImNodesStyle& style = ImNodes::GetStyle();
	style.NodeCornerRounding = 6.0f;
	style.PinCircleRadius = 5.0f;
	style.LinkThickness = 2.5f;

	ResetToDefault();
}

void BTNodeGraphEditor::Finalize()
{
	// コンテキストは ImGuiManager 側で破棄するため何もしない
}

//======================================================
// 毎フレーム描画
//======================================================
void BTNodeGraphEditor::Draw()
{

	// ===== メインウィンドウ =====
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
	ImGui::Begin("##BTEditorRoot",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	DrawMenuBar();

	// ===== レイアウト: 左 サイドパネル | 右 グラフキャンバス =====
	// ===== 左：グループパネル =====
	ImGui::BeginChild("##GroupPanel", ImVec2(210.0f, 0.0f), true);
	DrawGroupPanel();
	ImGui::EndChild();
	ImGui::SameLine();

	// ===== 中：ノード詳細パネル =====
	ImGui::BeginChild("##SidePanel", ImVec2(280.0f, 0.0f), true);
	DrawSidePanel();
	ImGui::EndChild();
	ImGui::SameLine();

	// ===== 右：グラフキャンバス =====
	ImGui::BeginChild("##GraphCanvas", ImVec2(0.0f, 0.0f), false);
	DrawNodeGraph();
	ImGui::EndChild();

	ImGui::End();
}

//======================================================
// メニューバー
//======================================================
void BTNodeGraphEditor::DrawMenuBar()
{
	if (!ImGui::BeginMenuBar()) return;

	if (ImGui::BeginMenu("File")) {
		// 保存パス入力
		static char pathBuf[256] = {};
		if (pathBuf[0] == '\0') {
			snprintf(pathBuf, sizeof(pathBuf), "%s", saveFilePath_.c_str());
		}
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputText("Path", pathBuf, sizeof(pathBuf));
		saveFilePath_ = pathBuf;

		if (ImGui::MenuItem("Save JSON", "Ctrl+S")) {
			if (SaveToJson(saveFilePath_)) {
				ImGui::OpenPopup("##SaveOk");
			}
		}
		if (ImGui::MenuItem("Load JSON", "Ctrl+O")) {
			LoadFromJson(saveFilePath_);
		}
		if (ImGui::MenuItem("Reset Graph")) {
			ResetToDefault();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Add Node")) {
		if (ImGui::MenuItem("Sequence  （順列)"))  AddNode(NodeKind::Sequence);
		if (ImGui::MenuItem("Selector  （選択)"))  AddNode(NodeKind::Selector);
		if (ImGui::MenuItem("Decorator （反転)")) AddNode(NodeKind::Decorator);
		if (ImGui::MenuItem("Leaf     （アクション)"))      AddNode(NodeKind::Leaf);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit")) {
		if (ImGui::MenuItem("選択ノードを削除", "Delete")) {
			DeleteSelectedNodes();
		}
		if (ImGui::MenuItem("選択リンクを削除")) {
			DeleteSelectedLinks();
		}
		ImGui::EndMenu();
	}

	// 自動整理 / グループ系ボタン
	ImGui::Separator();
	if (ImGui::Button(" 自動整理↓ ")) {
		AutoLayout();            // 上→下（縦方向）
	}
	ImGui::SetItemTooltip("ツリーを上から下へ整列（縦方向）");
	ImGui::SameLine();
	if (ImGui::Button(" 自動整理→ ")) {
		AutoLayoutHorizontal();  // 左→右（横方向）
	}
	ImGui::SetItemTooltip("ツリーを左から右へ整列（横方向）");
	ImGui::SameLine();
	if (ImGui::Button(" 自動グループ分け ")) {
		AutoGrouping();          // グループ生成のみ（整列はしない）
	}
	ImGui::SetItemTooltip("ツリーを解析してグループを自動生成");
	ImGui::SameLine();
	if (ImGui::Button(" グループ別整列 ")) {
		GroupLayout();           // グループ単位で縦にまとめる
	}
	ImGui::SetItemTooltip("同じグループのノードを縦にまとめて配置");
	ImGui::Separator();
	if (ImGui::Button("  ゲームに反映  ")) {
		if (applyCallback_) applyCallback_(graph_);
	}
	ImGui::SetItemTooltip("BTグラフを保存してゲームに反映します");

	ImGui::EndMenuBar();

	// 保存完了ポップアップ
	if (ImGui::BeginPopup("##SaveOk")) {
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
			"Saved: %s", saveFilePath_.c_str());
		if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

//======================================================
// imnodes グラフキャンバス
//======================================================
void BTNodeGraphEditor::DrawNodeGraph()
{
	// ===== スクロール可能なキャンバス領域 =====
	ImGui::BeginChild("##NodeCanvas",
		ImVec2(0.0f, 0.0f),
		false,
		ImGuiWindowFlags_HorizontalScrollbar);

	// ===== マウスホイールでズーム =====
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			// Ctrl を押していないときだけズーム（押しているときはデフォルトのスクロール）
			if (!ImGui::GetIO().KeyCtrl) {
				zoomScale_ += wheel * 0.08f;
				if (zoomScale_ < 0.3f) zoomScale_ = 0.3f; // ズーム範囲: 30%〜250%
				if (zoomScale_ > 2.5f) zoomScale_ = 2.5f;
			}
		}
	}

	// ===== ズームを FontScale で反映 =====
	ImGui::SetWindowFontScale(zoomScale_);

	ImNodes::BeginNodeEditor();


	// ===== ノードの描画 =====
	for (auto& node : graph_.nodes)
	{
		// ノード色を種別で決める（グループに属していればグループカラーで上書き）
		unsigned int baseColor = NodeColor(node.kind);
		unsigned int titleColor = NodeTitleColor(node.kind);
		int gid = GroupOfNode(node.id);
		if (gid != -1) {
			for (auto& grp : graph_.groups) {
				if (grp.id == gid) {
					// グループカラーをノード背景に薄く反映
					baseColor = IM_COL32(grp.colorR / 2 + 30,
						grp.colorG / 2 + 30,
						grp.colorB / 2 + 30, 255);
					titleColor = IM_COL32(grp.colorR * 2 / 3,
						grp.colorG * 2 / 3,
						grp.colorB * 2 / 3, 255);
					break;
				}
			}
		}
		ImNodes::PushColorStyle(ImNodesCol_NodeBackground, baseColor);
		ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered,
			baseColor + 0x00101010u);
		ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected,
			baseColor + 0x00202020u);
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, titleColor);
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, titleColor + 0x00101010u);
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, titleColor + 0x00202020u);

		ImNodes::BeginNode(node.ImNodeId());

		// --- タイトルバー ---
		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(node.label.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("[%s]", NodeKindName(node.kind));
		ImNodes::EndNodeTitleBar();

		// --- 入力ピン（Root 以外）---
		if (node.kind != NodeKind::Root) {
			ImNodes::BeginInputAttribute(node.InputPinId());
			ImGui::TextDisabled("in");
			ImNodes::EndInputAttribute();
		}

		// --- ノード本体コンテンツ ---
		if (node.kind == NodeKind::Leaf) {
			ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f),
				"%s", LeafStateTypeName(node.param.stateType));
		}

		// --- 出力ピン（Leaf 以外）---
		if (node.kind != NodeKind::Leaf) {
			ImNodes::BeginOutputAttribute(node.OutputPinId());
			ImGui::Indent(40.0f);
			ImGui::TextDisabled("out");
			ImNodes::EndOutputAttribute();
		}

		ImNodes::EndNode();

		ImNodes::PopColorStyle();  // TitleBarSelected
		ImNodes::PopColorStyle();  // TitleBarHovered
		ImNodes::PopColorStyle();  // TitleBar
		ImNodes::PopColorStyle();  // NodeBackgroundSelected
		ImNodes::PopColorStyle();  // NodeBackgroundHovered
		ImNodes::PopColorStyle();  // NodeBackground

		// imnodes にノード位置を反映（初期配置 / JSON 読込後）
		ImVec2 pos(node.posX, node.posY);
		ImNodes::SetNodeGridSpacePos(node.ImNodeId(), pos);
	}

	// ===== リンクの描画 =====
	for (auto& link : graph_.links)
	{
		const EditorNode* from = graph_.FindNode(link.fromNodeId);
		const EditorNode* to = graph_.FindNode(link.toNodeId);
		if (!from || !to) continue;

		ImNodes::Link(link.id, from->OutputPinId(), to->InputPinId());
	}

	// ===== 右クリック検出（BeginNodeEditor 内）=====
	DetectContextMenu();

	// ===== ミニマップ（EndNodeEditor の直前に呼ぶ）=====
	// 右下に表示、サイズはキャンバスの20%
	ImNodes::MiniMap(
		0.2f,                              // ミニマップのサイズ比率（20%）
		ImNodesMiniMapLocation_BottomRight // 表示位置：右下
	);

	ImNodes::EndNodeEditor();

	// ===== 右クリックポップアップ（EndNodeEditor 後）=====
	DrawContextMenuPopup();

	// FontScale を元に戻す（他のウィンドウに影響しないように）
	ImGui::SetWindowFontScale(1.0f);
	ImGui::EndChild(); // BeginChild に対応する EndChild

	// ===== ノード位置を毎フレーム graph_ へ書き戻す =====
	for (auto& node : graph_.nodes)
	{
		ImVec2 pos = ImNodes::GetNodeGridSpacePos(node.ImNodeId());
		node.posX = pos.x;
		node.posY = pos.y;
	}

	// ===== 新規リンク作成 =====
	{
		int startAttr = -1, endAttr = -1;
		if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
		{
			// ピンID → ノードID に逆引き
			int fromNodeId = -1, toNodeId = -1;
			for (auto& n : graph_.nodes) {
				if (n.OutputPinId() == startAttr) fromNodeId = n.id;
				if (n.InputPinId() == endAttr)   toNodeId = n.id;
			}
			if (fromNodeId != -1 && toNodeId != -1) {
				AddLink(fromNodeId, toNodeId);
			}
		}
	}

	// ===== リンク削除（ホバー + Delete） =====
	{
		int hoveredLink = -1;
		if (ImNodes::IsLinkHovered(&hoveredLink) &&
			ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			DeleteLink(hoveredLink);
		}
	}

	// ===== 選択ノードを Delete で削除 =====
	if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive())
	{
		DeleteSelectedNodes();
		DeleteSelectedLinks();
	}

	// ===== クリックで選択ノードをサイドパネルに反映 =====
	{
		int hoveredNode = -1;
		if (ImNodes::IsNodeHovered(&hoveredNode)) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				// ImNodeId → EditorNode.id に逆引き
				for (auto& n : graph_.nodes) {
					if (n.ImNodeId() == hoveredNode) {
						selectedNodeId_ = n.id;
						break;
					}
				}
			}
		}
	}
}

//======================================================
// 右クリックコンテキストメニュー
//======================================================
// BeginNodeEditor 内：右クリック検出のみ
void BTNodeGraphEditor::DetectContextMenu()
{
	int hoveredNode = -1, hoveredLink = -1;
	if (ImNodes::IsEditorHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
		!ImNodes::IsNodeHovered(&hoveredNode) &&
		!ImNodes::IsLinkHovered(&hoveredLink))
	{
		ImVec2 mp = ImGui::GetMousePos();
		contextMenuPosX_ = mp.x;
		contextMenuPosY_ = mp.y;
		ImGui::OpenPopup("##CanvasContextMenu");
	}
}

// EndNodeEditor 後：ポップアップ描画
void BTNodeGraphEditor::DrawContextMenuPopup()
{
	if (ImGui::BeginPopup("##CanvasContextMenu"))
	{
		ImGui::TextDisabled("ノードを追加");
		ImGui::Separator();

		ImVec2 panning = ImNodes::EditorContextGetPanning();
		ImVec2 editorWinPos = ImGui::GetWindowPos();
		float gx = (contextMenuPosX_ - editorWinPos.x) - panning.x;
		float gy = (contextMenuPosY_ - editorWinPos.y) - panning.y;

		auto AddAt = [&](NodeKind k) {
			AddNode(k, gx, gy);
			ImGui::CloseCurrentPopup();
			};

		if (ImGui::MenuItem("Sequence  （順列)"))  AddAt(NodeKind::Sequence);
		if (ImGui::MenuItem("Selector  （選択)"))  AddAt(NodeKind::Selector);
		if (ImGui::MenuItem("Decorator （反転)")) AddAt(NodeKind::Decorator);
		if (ImGui::MenuItem("Leaf     （アクション)"))      AddAt(NodeKind::Leaf);
		ImGui::Separator();
		if (ImGui::MenuItem("選択ノードを削除")) { DeleteSelectedNodes(); ImGui::CloseCurrentPopup(); }

		ImGui::EndPopup();
	}
}

//======================================================
// AutoLayoutHorizontal
// ツリー構造を解析して左→右へ自動整列する
// ・X軸：深さ（Rootから何段目か）に応じて右へ配置
// ・Y軸：兄弟ノードを等間隔に縦並び
// ・親ノードは子ノード群の中央（縦）に配置
//======================================================
void BTNodeGraphEditor::AutoLayoutHorizontal()
{
	using namespace BTEditor;

	constexpr float kNodeSpacingX = 260.0f;
	constexpr float kNodeSpacingY = 140.0f;
	constexpr float kOriginX = 60.0f;
	constexpr float kOriginY = 60.0f;

	EditorNode* rootNode = nullptr;
	for (auto& n : graph_.nodes) {
		if (n.kind == NodeKind::Root) { rootNode = &n; break; }
	}

	if (!rootNode) return;

	int leafCounter = 0;

	std::function<float(int, int)> CalcY =
		[&](int nodeId, int depth) -> float
		{
			std::vector<int> children = graph_.ChildrenOf(nodeId);
			EditorNode* node = graph_.FindNode(nodeId);
			if (!node) return 0.0f;

			node->posX = kOriginX + depth * kNodeSpacingX;

			if (children.empty()) {
				node->posY = kOriginY + leafCounter * kNodeSpacingY;
				++leafCounter;
				return node->posY;
			}

			float firstChildY = 0.0f, lastChildY = 0.0f;
			bool  first = true;
			for (int cid : children) {
				float cy = CalcY(cid, depth + 1);
				if (first) { firstChildY = cy; first = false; }
				lastChildY = cy;
			}
			node->posY = (firstChildY + lastChildY) * 0.5f;
			return node->posY;
		};

	CalcY(rootNode->id, 0);

	for (auto& node : graph_.nodes) {
		ImNodes::SetNodeGridSpacePos(
			node.ImNodeId(), ImVec2(node.posX, node.posY));
	}
}


//======================================================
// サイドパネル（選択ノードのパラメーター編集）
//======================================================
void BTNodeGraphEditor::DrawSidePanel()
{
	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "BT Node Editor");
	ImGui::Separator();

	// グラフ統計
	// ズームリセットボタン
	ImGui::Text("ズーム: %.0f%%", zoomScale_ * 100.0f);
	ImGui::SameLine();
	if (ImGui::SmallButton("リセット")) { zoomScale_ = 1.0f; }
	ImGui::Separator();
	ImGui::Text("Nodes: %d  Links: %d",
		(int)graph_.nodes.size(),
		(int)graph_.links.size());
	ImGui::Spacing();

	// ノードリスト
	ImGui::TextDisabled("-- Node List --");
	for (auto& n : graph_.nodes) {
		bool sel = (n.id == selectedNodeId_);
		char label[64];
		snprintf(label, sizeof(label), "[%d] %s", n.id, n.label.c_str());
		if (ImGui::Selectable(label, sel)) {
			selectedNodeId_ = n.id;
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

	// 選択ノードのパラメーター
	EditorNode* node = graph_.FindNode(selectedNodeId_);
	if (!node) {
		ImGui::TextDisabled("(ノードを選択してください)");
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Edit: [%d] %s", node->id, node->label.c_str());
	ImGui::Separator();

	// ラベル編集
	char labelBuf[64] = {};
	snprintf(labelBuf, sizeof(labelBuf), "%s", node->label.c_str());
	if (ImGui::InputText("ラベル名", labelBuf, sizeof(labelBuf))) {
		node->label = labelBuf;
	}

	ImGui::Spacing();
	DrawParamEditor(*node);

	// 削除ボタン（Root は削除不可）
	ImGui::Spacing();
	ImGui::Separator();
	if (node->kind != NodeKind::Root) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
		if (ImGui::Button("このノードを削除")) {
			DeleteNode(node->id);
			selectedNodeId_ = -1;
		}
		ImGui::PopStyleColor();
	}
}

//======================================================
// パラメーター編集 UI
//======================================================
void BTNodeGraphEditor::DrawParamEditor(EditorNode& node)
{
	switch (node.kind)
	{
	case NodeKind::Root:
		ImGui::TextDisabled("Root ノードはパラメーターなし");
		break;

	case NodeKind::Sequence:
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Sequence");
		ImGui::TextWrapped("子ノードを左→右の順に実行し、\n全て Success なら Success。");
		break;

	case NodeKind::Selector:
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Selector");
		ImGui::TextWrapped("子ノードを左→右の順に実行し、\n1つでも Success なら Success。");
		break;

	case NodeKind::Decorator:
		ImGui::TextColored(ImVec4(0.9f, 0.5f, 1.0f, 1.0f), "Decorator");
		{
			static const char* kDecTypes[] = { "Inverter" };
			int cur = 0;
			if (node.param.decoratorType == "Inverter") cur = 0;
			if (ImGui::Combo("種別##dec", &cur, kDecTypes, IM_ARRAYSIZE(kDecTypes))) {
				node.param.decoratorType = kDecTypes[cur];
			}
		}
		break;

	case NodeKind::Leaf:
	{
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Leaf");

		// ステート種別コンボ
		static const char* kStateNames[] = {
			"None",
			"ChargeDash       (突撃ダッシュ)",
			"SummonMinion     (雑魚召喚)",
			"FindTarget       (ターゲット探索)",
			"ChaseTarget      (追跡)",
			"NearIdle         (近距離待機)",
			"StayHome         (定点待機)",
			"IsTargetFar      (遠距離判定)",
			"ShootSplitBullet (分裂弾発射)",
			"ThrowBigBullet   (大弾投げ)",
			"IsPhase2         (HP50%以下)",
			"IsPhase3         (HP25%以下)",
			"Custom",
		};
		int cur = static_cast<int>(node.param.stateType);
		if (ImGui::Combo("アクション種別", &cur, kStateNames, IM_ARRAYSIZE(kStateNames))) {
			node.param.stateType = static_cast<LeafStateType>(cur);
			static const char* kLabelNames[] = {
				"None","ChargeDash","SummonMinion","FindTarget","ChaseTarget",
				"NearIdle","StayHome","IsTargetFar","ShootSplitBullet","ThrowBigBullet","IsPhase2","IsPhase3","NotLastAction","IsAngry","ShootSplitBulletAngry","IdleWait","Custom"
			};
			if (cur < IM_ARRAYSIZE(kLabelNames)) node.label = kLabelNames[cur];
		}

		ImGui::Spacing();

		if (node.param.stateType == LeafStateType::ThrowBigBullet) {
			ImGui::TextWrapped("大弾を1発プレイヤーに向けて投げます。\n溜め1.5秒 → 投擲 → 硬直1.0秒\nフェーズ2以降推奨");
		}
		if (node.param.stateType == LeafStateType::IsPhase2) {
			ImGui::TextWrapped("HP50%%以下なら Success\nSequenceの条件として使用");
		}
		if (node.param.stateType == LeafStateType::IsPhase3) {
			ImGui::TextWrapped("HP25%%以下なら Success\nSequenceの条件として使用");
		}
		if (node.param.stateType == LeafStateType::IdleWait) {
			ImGui::TextWrapped("0.8秒その場で待機します。攻撃の合間の隙として使います。Selectorの最後に置くのがオススメ。");
		}
		if (node.param.stateType == LeafStateType::IsAngry) {
			ImGui::TextWrapped("HP50%%以下（怒り状態）なら Success。\nSequenceの条件として使用。");
		}
		if (node.param.stateType == LeafStateType::ShootSplitBulletAngry) {
			ImGui::TextWrapped("怒り時の分裂弾（8発）。\n溜め0.6秒 → 発射 → 硬直0.8秒");
		}
		if (node.param.stateType == LeafStateType::NotLastAction) {
			ImGui::TextWrapped("前回と同じ攻撃なら Fail。Sequenceの先頭に置いて連続使用を防ぎます。");
			ImGui::Spacing();
			char buf[64] = {};
			snprintf(buf, sizeof(buf), "%s", node.param.lastActionName.c_str());
			if (ImGui::InputText("禁止する攻撃名", buf, sizeof(buf))) {
				node.param.lastActionName = buf;
				node.label = std::string("Not:") + buf;
			}
			ImGui::TextDisabled("例: ChargeDash / ShootSplitBullet / ThrowBigBullet");
		}
		if (node.param.stateType == LeafStateType::ShootSplitBullet) {
			ImGui::TextWrapped("分裂弾を4発プレイヤーに向けて発射します。\n溜め0.6秒 → 発射 → 硬直0.8秒");
		}

		// ChargeDash 専用パラメーター
		if (node.param.stateType == LeafStateType::ChargeDash) {
			DrawChargeDashParamEditor(node.param.chargeDash);
		}

		// 距離系
		if (node.param.stateType == LeafStateType::IsTargetFar ||
			node.param.stateType == LeafStateType::ChaseTarget)
		{
			ImGui::DragFloat("距離しきい値",
				&node.param.thresholdDistance,
				0.1f, 0.0f, 200.0f, "%.2f m");
		}

		// カスタム名
		if (node.param.stateType == LeafStateType::Custom) {
			char buf[64] = {};
			snprintf(buf, sizeof(buf), "%s", node.param.customName.c_str());
			if (ImGui::InputText("Custom Name", buf, sizeof(buf))) {
				node.param.customName = buf;
				node.label = buf;
			}
		}
		break;
	}

	default:
		break;
	}
}

void BTNodeGraphEditor::DrawChargeDashParamEditor(ChargeDashParamData& p)
{
	if (!ImGui::CollapsingHeader("チャージダッシュ パラメーター", ImGuiTreeNodeFlags_DefaultOpen)) return;

	ImGui::PushItemWidth(120.0f);
	ImGui::DragFloat("溜め時間", &p.chargeTime, 0.01f, 0.0f, 10.0f, "%.2f s");
	ImGui::DragFloat("ダッシュ距離", &p.dashDistance, 0.5f, 0.0f, 200.0f, "%.1f m");
	ImGui::DragFloat("ダッシュ速度", &p.dashSpeed, 0.5f, 0.0f, 200.0f, "%.1f m/s");
	ImGui::DragFloat("硬直時間", &p.recoverTime, 0.01f, 0.0f, 10.0f, "%.2f s");
	ImGui::DragFloat("クールダウン", &p.cooldownTime, 0.01f, 0.0f, 30.0f, "%.2f s");
	ImGui::DragFloat("旋回補間", &p.turnLerp, 0.01f, 0.0f, 1.0f, "%.2f");
	ImGui::PopItemWidth();
}

//======================================================
// ノード操作
//======================================================
int BTNodeGraphEditor::AddNode(NodeKind kind, float posX, float posY)
{
	EditorNode n;
	n.id = graph_.NewId();
	n.kind = kind;
	n.posX = posX;
	n.posY = posY;

	switch (kind) {
	case NodeKind::Root:      n.label = "Root";     break;
	case NodeKind::Sequence:  n.label = "Sequence"; break;
	case NodeKind::Selector:  n.label = "Selector"; break;
	case NodeKind::Decorator: n.label = "Inverter";  break;
	case NodeKind::Leaf:      n.label = "Leaf";     break;
	default:                  n.label = "Node";     break;
	}

	graph_.nodes.push_back(n);
	selectedNodeId_ = n.id;

	// imnodes に初期位置を設定
	ImNodes::SetNodeGridSpacePos(n.ImNodeId(), ImVec2(posX, posY));

	return n.id;
}

bool BTNodeGraphEditor::AddLink(int fromNodeId, int toNodeId)
{
	// 自己ループ禁止
	if (fromNodeId == toNodeId) return false;

	// 循環チェック
	if (WouldCreateCycle(fromNodeId, toNodeId)) return false;

	// 同じリンクの多重登録防止
	for (auto& l : graph_.links) {
		if (l.fromNodeId == fromNodeId && l.toNodeId == toNodeId) return false;
	}

	// toNode が既に親を持つ場合は上書き（1親制約）
	for (auto it = graph_.links.begin(); it != graph_.links.end(); ++it) {
		if (it->toNodeId == toNodeId) {
			graph_.links.erase(it);
			break;
		}
	}

	EditorLink link;
	link.id = graph_.NewId();
	link.fromNodeId = fromNodeId;
	link.toNodeId = toNodeId;
	graph_.links.push_back(link);
	return true;
}

void BTNodeGraphEditor::DeleteNode(int nodeId)
{
	// Root は削除禁止
	const EditorNode* n = graph_.FindNode(nodeId);
	if (!n || n->kind == NodeKind::Root) return;

	// 関連リンクを全削除
	graph_.links.erase(
		std::remove_if(graph_.links.begin(), graph_.links.end(),
			[nodeId](const EditorLink& l) {
				return l.fromNodeId == nodeId || l.toNodeId == nodeId;
			}),
		graph_.links.end());

	// ノード本体を削除
	graph_.nodes.erase(
		std::remove_if(graph_.nodes.begin(), graph_.nodes.end(),
			[nodeId](const EditorNode& n) { return n.id == nodeId; }),
		graph_.nodes.end());

	if (selectedNodeId_ == nodeId) selectedNodeId_ = -1;
}

void BTNodeGraphEditor::DeleteLink(int linkId)
{
	graph_.links.erase(
		std::remove_if(graph_.links.begin(), graph_.links.end(),
			[linkId](const EditorLink& l) { return l.id == linkId; }),
		graph_.links.end());
}

void BTNodeGraphEditor::DeleteSelectedNodes()
{
	// imnodes で選択中のノード ImNodeId を取得
	int count = ImNodes::NumSelectedNodes();
	if (count <= 0) return;

	std::vector<int> selectedImIds(count);
	ImNodes::GetSelectedNodes(selectedImIds.data());

	// ImNodeId → EditorNode.id に変換して削除
	for (int imId : selectedImIds) {
		for (auto& n : graph_.nodes) {
			if (n.ImNodeId() == imId) {
				DeleteNode(n.id);
				break;
			}
		}
	}
}

void BTNodeGraphEditor::DeleteSelectedLinks()
{
	int count = ImNodes::NumSelectedLinks();
	if (count <= 0) return;

	std::vector<int> selectedIds(count);
	ImNodes::GetSelectedLinks(selectedIds.data());
	for (int id : selectedIds) {
		DeleteLink(id);
	}
}

//======================================================
// 循環チェック（DFS で toNodeId が fromNodeId の祖先か調べる）
//======================================================
bool BTNodeGraphEditor::WouldCreateCycle(int fromNodeId, int toNodeId) const
{
	// fromNodeId の祖先（親、祖父…）に toNodeId がいれば循環
	int cur = fromNodeId;
	while (cur != -1) {
		if (cur == toNodeId) return true;
		cur = graph_.ParentOf(cur);
	}
	return false;
}

//======================================================
// ノード色
//======================================================
unsigned int BTNodeGraphEditor::NodeColor(NodeKind kind) const
{
	switch (kind) {
	case NodeKind::Root:      return kColorRoot;
	case NodeKind::Sequence:  return kColorSeq;
	case NodeKind::Selector:  return kColorSel;
	case NodeKind::Decorator: return kColorDec;
	case NodeKind::Leaf:      return kColorLeaf;
	default:                  return IM_COL32(80, 80, 80, 255);
	}
}

unsigned int BTNodeGraphEditor::NodeTitleColor(NodeKind kind) const
{
	switch (kind) {
	case NodeKind::Root:      return kColorRootTitle;
	case NodeKind::Sequence:  return kColorSeqTitle;
	case NodeKind::Selector:  return kColorSelTitle;
	case NodeKind::Decorator: return kColorDecTitle;
	case NodeKind::Leaf:      return kColorLeafTitle;
	default:                  return IM_COL32(50, 50, 50, 255);
	}
}

//======================================================
// DrawGroupPanel
// 左ペイン：グループ一覧・折りたたみ・カメラ移動
//======================================================
void BTNodeGraphEditor::DrawGroupPanel()
{
	using namespace BTEditor;

	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "グループ管理");
	ImGui::Separator();

	// ===== 新規グループ作成 =====
	static char newGroupName[64] = "新しいグループ";
	static int  newColorIdx = 0;
	// プリセットカラー（名前付き）
	struct ColorPreset { const char* name; int r, g, b; };
	static const ColorPreset kColors[] = {
		{"赤",    180, 60,  60 },
		{"青",    60,  100, 180},
		{"緑",    60,  160, 80 },
		{"紫",    140, 60,  180},
		{"橙",    200, 120, 40 },
		{"水色",  60,  180, 180},
		{"灰",    120, 120, 120},
	};
	constexpr int kColorCount = 7;

	ImGui::SetNextItemWidth(130.0f);
	ImGui::InputText("##grpname", newGroupName, sizeof(newGroupName));
	ImGui::SetNextItemWidth(80.0f);
	ImGui::Combo("##grpcolor", &newColorIdx,
		[](void* data, int idx, const char** out) -> bool {
			*out = ((const ColorPreset*)data)[idx].name;
			return true;
		}, (void*)kColors, kColorCount);
	ImGui::SameLine();
	if (ImGui::Button("+追加")) {
		const auto& col = kColors[newColorIdx];
		AddGroup(newGroupName, col.r, col.g, col.b);
	}

	ImGui::Separator();
	ImGui::Spacing();

	// ===== グループ一覧 =====
	if (graph_.groups.empty()) {
		ImGui::TextDisabled("グループなし");
		ImGui::TextDisabled("ノードを選択して");
		ImGui::TextDisabled("グループに追加できます");
	}

	for (auto& grp : graph_.groups)
	{
		// グループカラーをラベルに反映
		ImVec4 col(grp.colorR / 255.0f,
			grp.colorG / 255.0f,
			grp.colorB / 255.0f, 1.0f);

		// 折りたたみヘッダー
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(col.x * 0.6f, col.y * 0.6f, col.z * 0.6f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(col.x * 0.8f, col.y * 0.8f, col.z * 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, col);

		bool open = ImGui::CollapsingHeader(
			grp.name.c_str(),
			ImGuiTreeNodeFlags_DefaultOpen);

		ImGui::PopStyleColor(3);

		// ヘッダー右クリックでグループ削除
		if (ImGui::IsItemHovered() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(("##grpdel" + std::to_string(grp.id)).c_str());
		}
		if (ImGui::BeginPopup(("##grpdel" + std::to_string(grp.id)).c_str())) {
			ImGui::TextColored(col, "%s", grp.name.c_str());
			ImGui::Separator();

			// ===== グループ名の変更 =====
			static char renameBuf[64] = {};
			if (ImGui::IsWindowAppearing()) {
				snprintf(renameBuf, sizeof(renameBuf), "%s", grp.name.c_str());
			}
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::InputText("名前##rename", renameBuf, sizeof(renameBuf),
				ImGuiInputTextFlags_EnterReturnsTrue)) {
				grp.name = renameBuf;
			}

			ImGui::Separator();

			// ===== カラー変更 =====
			ImGui::Text("カラー変更:");
			// プリセットカラーボタン（横並び）
			struct CPre { const char* name; int r, g, b; };
			static const CPre kPre[] = {
				{"赤",   200, 60,  60 },
				{"橙",   200, 130, 40 },
				{"黄",   200, 190, 40 },
				{"緑",   60,  160, 80 },
				{"水",   60,  180, 180},
				{"青",   60,  100, 200},
				{"紫",   140, 60,  180},
				{"灰",   120, 120, 120},
			};
			for (int pi = 0; pi < 8; ++pi) {
				if (pi % 4 != 0) ImGui::SameLine();
				ImVec4 pc(
					kPre[pi].r / 255.0f,
					kPre[pi].g / 255.0f,
					kPre[pi].b / 255.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Button, pc);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					ImVec4(pc.x * 1.2f, pc.y * 1.2f, pc.z * 1.2f, 1.0f));
				char pid[16];
				snprintf(pid, sizeof(pid), "%s##p%d_%d", kPre[pi].name, grp.id, pi);
				if (ImGui::Button(pid, ImVec2(36, 22))) {
					grp.colorR = kPre[pi].r;
					grp.colorG = kPre[pi].g;
					grp.colorB = kPre[pi].b;
				}
				ImGui::PopStyleColor(2);
			}

			// RGB スライダーで細かく調整
			ImGui::Spacing();
			float fc[3] = {
				grp.colorR / 255.0f,
				grp.colorG / 255.0f,
				grp.colorB / 255.0f
			};
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::ColorEdit3(
				("##col" + std::to_string(grp.id)).c_str(),
				fc,
				ImGuiColorEditFlags_NoLabel |
				ImGuiColorEditFlags_NoAlpha)) {
				grp.colorR = (int)(fc[0] * 255);
				grp.colorG = (int)(fc[1] * 255);
				grp.colorB = (int)(fc[2] * 255);
			}

			ImGui::Separator();
			if (ImGui::MenuItem("カメラをこのグループへ移動")) {
				FocusGroup(grp.id);
			}
			if (ImGui::MenuItem("このグループを削除")) {
				DeleteGroup(grp.id);
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				break;
			}
			ImGui::EndPopup();
		}

		if (!open) continue;

		// グループ内のノード一覧
		for (int nid : grp.nodeIds)
		{
			EditorNode* n = graph_.FindNode(nid);
			if (!n) continue;

			bool sel = (n->id == selectedNodeId_);
			char lbl[64];
			snprintf(lbl, sizeof(lbl), "  [%d] %s", n->id, n->label.c_str());

			ImGui::PushStyleColor(ImGuiCol_Text, col);
			if (ImGui::Selectable(lbl, sel,
				ImGuiSelectableFlags_None, ImVec2(0, 0))) {
				selectedNodeId_ = n->id;
				// クリックでカメラをそのノードへ移動
				ImNodes::SetNodeGridSpacePos(
					n->ImNodeId(), ImVec2(n->posX, n->posY));
			}
			ImGui::PopStyleColor();

			// ノードをグループから除外ボタン
			ImGui::SameLine();
			char btnId[32];
			snprintf(btnId, sizeof(btnId), "x##rm%d_%d", grp.id, nid);
			ImGui::PushStyleColor(ImGuiCol_Button,
				ImVec4(0.5f, 0.1f, 0.1f, 0.6f));
			if (ImGui::SmallButton(btnId)) {
				grp.nodeIds.erase(
					std::remove(grp.nodeIds.begin(),
						grp.nodeIds.end(), nid),
					grp.nodeIds.end());
				break;
			}
			ImGui::PopStyleColor();
		}

		// 選択中ノードをこのグループに追加
		if (selectedNodeId_ != -1) {
			// 既に属していなければ追加ボタン表示
			bool already = std::find(grp.nodeIds.begin(),
				grp.nodeIds.end(), selectedNodeId_) != grp.nodeIds.end();
			if (!already) {
				char addBtn[64];
				snprintf(addBtn, sizeof(addBtn),
					" + 選択ノード[%d]を追加##%d",
					selectedNodeId_, grp.id);
				ImGui::PushStyleColor(ImGuiCol_Button,
					ImVec4(col.x * 0.4f, col.y * 0.4f, col.z * 0.4f, 1.0f));
				if (ImGui::SmallButton(addBtn)) {
					grp.nodeIds.push_back(selectedNodeId_);
				}
				ImGui::PopStyleColor();
			}
		}

		ImGui::Spacing();
	}
}

//======================================================
// AddGroup / DeleteGroup / FocusGroup / GroupOfNode
//======================================================
void BTNodeGraphEditor::AddGroup(const std::string& name, int r, int g, int b)
{
	BTEditor::NodeGroup grp;
	grp.id = graph_.NewId();
	grp.name = name;
	grp.colorR = r;
	grp.colorG = g;
	grp.colorB = b;
	graph_.groups.push_back(grp);
}

void BTNodeGraphEditor::DeleteGroup(int groupId)
{
	graph_.groups.erase(
		std::remove_if(graph_.groups.begin(), graph_.groups.end(),
			[groupId](const BTEditor::NodeGroup& g) {
				return g.id == groupId;
			}),
		graph_.groups.end());
	if (selectedGroupId_ == groupId) selectedGroupId_ = -1;
}

void BTNodeGraphEditor::FocusGroup(int groupId)
{
	for (auto& grp : graph_.groups) {
		if (grp.id != groupId) continue;
		if (grp.nodeIds.empty()) return;

		// グループ内ノードの重心を計算してカメラを移動
		float cx = 0.0f, cy = 0.0f;
		int   count = 0;
		for (int nid : grp.nodeIds) {
			const BTEditor::EditorNode* n = graph_.FindNode(nid);
			if (!n) continue;
			cx += n->posX;
			cy += n->posY;
			++count;
		}
		if (count == 0) return;
		cx /= count;
		cy /= count;

		// パンを調整してグループ中心を画面中央に
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		ImNodes::EditorContextResetPanning(
			ImVec2(canvasSize.x * 0.5f - cx,
				canvasSize.y * 0.5f - cy));
		break;
	}
}

int BTNodeGraphEditor::GroupOfNode(int nodeId) const
{
	for (auto& grp : graph_.groups) {
		for (int nid : grp.nodeIds) {
			if (nid == nodeId) return grp.id;
		}
	}
	return -1;
}


//======================================================
// AutoGrouping
//======================================================
void BTNodeGraphEditor::AutoGrouping()
{
	using namespace BTEditor;

	// 「[AUTO]」プレフィックスのグループだけ削除
	graph_.groups.erase(
		std::remove_if(graph_.groups.begin(), graph_.groups.end(),
			[](const NodeGroup& g) {
				return g.name.find("[AUTO]") == 0;
			}),
		graph_.groups.end());

	struct GroupDef {
		const char* name;
		int r, g, b;
		LeafStateType triggerState;
	};

	const std::vector<GroupDef> kGroupDefs = {
		{"[AUTO] 怒り状態（HP50%以下）", 200, 60,  60,  LeafStateType::IsAngry},
		{"[AUTO] フェーズ3（HP25%以下）", 200, 130, 40,  LeafStateType::IsPhase3},
		{"[AUTO] フェーズ2（HP50%以下）", 60,  120, 200, LeafStateType::IsPhase2},
	};

	std::vector<int> commonIds;
	std::vector<int> chaseIds;
	std::vector<int> rootIds;
	std::vector<std::vector<int>> groupNodeIds(kGroupDefs.size());

	std::function<void(int, std::vector<LeafStateType>)> Classify =
		[&](int nodeId, std::vector<LeafStateType> ancestorStates)
		{
			const EditorNode* en = graph_.FindNode(nodeId);
			if (!en) return;

			std::vector<LeafStateType> myAncestors = ancestorStates;
			if (en->kind == NodeKind::Leaf) {
				myAncestors.push_back(en->param.stateType);
			}

			if (en->kind == NodeKind::Leaf) {
				if (en->param.stateType == LeafStateType::FindTarget) {
					commonIds.push_back(nodeId);
					return;
				}
				if (en->param.stateType == LeafStateType::ChaseTarget) {
					chaseIds.push_back(nodeId);
					return;
				}
			}

			int matchedGroup = -1;
			for (int gi = 0; gi < (int)kGroupDefs.size(); ++gi) {
				for (auto& st : ancestorStates) {
					if (st == kGroupDefs[gi].triggerState) {
						matchedGroup = gi;
						break;
					}
				}
				if (matchedGroup != -1) break;
			}

			if (matchedGroup != -1) {
				groupNodeIds[matchedGroup].push_back(nodeId);
			}
			else if (en->kind == NodeKind::Root ||
				en->kind == NodeKind::Sequence ||
				en->kind == NodeKind::Selector ||
				en->kind == NodeKind::Decorator) {
				rootIds.push_back(nodeId);
			}

			for (int cid : graph_.ChildrenOf(nodeId)) {
				Classify(cid, myAncestors);
			}
		};

	for (auto& n : graph_.nodes) {
		if (n.kind == NodeKind::Root) {
			Classify(n.id, {});
			break;
		}
	}

	for (int gi = 0; gi < (int)kGroupDefs.size(); ++gi) {
		if (groupNodeIds[gi].empty()) continue;
		const auto& def = kGroupDefs[gi];
		NodeGroup grp;
		grp.id = graph_.NewId();
		grp.name = def.name;
		grp.colorR = def.r;
		grp.colorG = def.g;
		grp.colorB = def.b;
		grp.nodeIds = groupNodeIds[gi];
		graph_.groups.push_back(grp);
	}

	if (!commonIds.empty()) {
		NodeGroup grp;
		grp.id = graph_.NewId();
		grp.name = "[AUTO] 共通処理";
		grp.colorR = 120; grp.colorG = 120; grp.colorB = 120;
		grp.nodeIds = commonIds;
		graph_.groups.push_back(grp);
	}

	if (!chaseIds.empty()) {
		NodeGroup grp;
		grp.id = graph_.NewId();
		grp.name = "[AUTO] 追跡";
		grp.colorR = 60; grp.colorG = 180; grp.colorB = 180;
		grp.nodeIds = chaseIds;
		graph_.groups.push_back(grp);
	}

	if (!rootIds.empty()) {
		NodeGroup grp;
		grp.id = graph_.NewId();
		grp.name = "[AUTO] 構造ノード";
		grp.colorR = 80; grp.colorG = 80; grp.colorB = 80;
		grp.nodeIds = rootIds;
		grp.collapsed = true;
		graph_.groups.push_back(grp);
	}
}


//======================================================
// GroupLayout
//======================================================
void BTNodeGraphEditor::GroupLayout()
{
	using namespace BTEditor;

	constexpr float kGroupWidth = 260.0f;
	constexpr float kNodeSpacingY = 150.0f;
	constexpr float kOriginX = 80.0f;
	constexpr float kOriginY = 80.0f;
	constexpr float kGroupGap = 40.0f;

	if (graph_.groups.empty()) return;

	std::unordered_map<int, int> nodeToGroupIndex;
	for (int gi = 0; gi < (int)graph_.groups.size(); ++gi) {
		for (int nid : graph_.groups[gi].nodeIds) {
			nodeToGroupIndex[nid] = gi;
		}
	}

	std::vector<float> groupY(graph_.groups.size(), kOriginY);
	std::vector<float> groupX(graph_.groups.size(), 0.0f);
	float curX = kOriginX;
	int   structGroupIdx = -1;
	for (int gi = 0; gi < (int)graph_.groups.size(); ++gi) {
		if (graph_.groups[gi].name == "[AUTO] 構造ノード") {
			structGroupIdx = gi;
			continue;
		}
		groupX[gi] = curX;
		curX += kGroupWidth + kGroupGap;
	}
	if (structGroupIdx != -1) {
		groupX[structGroupIdx] = curX;
		curX += kGroupWidth + kGroupGap;
	}

	float ungroupedX = curX;
	float ungroupedY = kOriginY;

	for (auto& node : graph_.nodes) {
		auto it = nodeToGroupIndex.find(node.id);
		if (it == nodeToGroupIndex.end()) {
			node.posX = ungroupedX;
			node.posY = ungroupedY;
			ungroupedY += kNodeSpacingY;
		}
		else {
			int gi = it->second;
			node.posX = groupX[gi];
			node.posY = groupY[gi];
			groupY[gi] += kNodeSpacingY;
		}
	}

	for (auto& node : graph_.nodes) {
		ImNodes::SetNodeGridSpacePos(
			node.ImNodeId(),
			ImVec2(node.posX, node.posY));
	}
}


//======================================================
// AutoLayout
//======================================================
void BTNodeGraphEditor::AutoLayout()
{
	using namespace BTEditor;

	constexpr float kNodeSpacingX = 240.0f;
	constexpr float kNodeSpacingY = 160.0f;
	constexpr float kOriginX = 100.0f;
	constexpr float kOriginY = 60.0f;

	EditorNode* rootNode = nullptr;
	for (auto& n : graph_.nodes) {
		if (n.kind == NodeKind::Root) { rootNode = &n; break; }
	}
	if (!rootNode) return;

	int leafCounter = 0;

	std::function<float(int, int)> CalcX =
		[&](int nodeId, int depth) -> float
		{
			std::vector<int> children = graph_.ChildrenOf(nodeId);

			EditorNode* node = graph_.FindNode(nodeId);
			if (!node) return 0.0f;

			node->posY = kOriginY + depth * kNodeSpacingY;

			if (children.empty()) {
				node->posX = kOriginX + leafCounter * kNodeSpacingX;
				++leafCounter;
				return node->posX;
			}

			float firstChildX = 0.0f;
			float lastChildX = 0.0f;
			bool  first = true;

			for (int cid : children) {
				float cx = CalcX(cid, depth + 1);
				if (first) { firstChildX = cx; first = false; }
				lastChildX = cx;
			}

			node->posX = (firstChildX + lastChildX) * 0.5f;
			return node->posX;
		};

	CalcX(rootNode->id, 0);

	for (auto& node : graph_.nodes) {
		ImNodes::SetNodeGridSpacePos(
			node.ImNodeId(),
			ImVec2(node.posX, node.posY));
	}
}


void BTNodeGraphEditor::ResetToDefault()
{
	graph_.Clear();
	selectedNodeId_ = -1;

	EditorNode root;
	root.id = graph_.NewId();
	root.kind = NodeKind::Root;
	root.label = "Root";
	root.posX = 400.0f;
	root.posY = 100.0f;
	graph_.nodes.push_back(root);
}

//======================================================
// JSON 保存
//======================================================
bool BTNodeGraphEditor::SaveToJson(const std::string& filepath)
{
	try {
		nlohmann::json j = graph_;
		std::ofstream ofs(filepath);
		if (!ofs) return false;
		ofs << j.dump(2);
		return true;
	}
	catch (...) {
		return false;
	}
}

//======================================================
// JSON 読み込み
//======================================================
bool BTNodeGraphEditor::LoadFromJson(const std::string& filepath)
{
	try {
		std::ifstream ifs(filepath);
		if (!ifs) return false;
		nlohmann::json j;
		ifs >> j;
		graph_ = j.get<BTEditor::BTGraph>();
		selectedNodeId_ = -1;

		// imnodes に位置を再設定
		for (auto& n : graph_.nodes) {
			ImNodes::SetNodeGridSpacePos(n.ImNodeId(), ImVec2(n.posX, n.posY));
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

#else // USE_IMGUI 未定義時（Release）：空実装でリンクエラーを防ぐ

BTNodeGraphEditor::BTNodeGraphEditor() = default;
BTNodeGraphEditor::~BTNodeGraphEditor() = default;
void BTNodeGraphEditor::Initialize() {}
void BTNodeGraphEditor::Finalize() {}
void BTNodeGraphEditor::Draw() {}
bool BTNodeGraphEditor::SaveToJson(const std::string&) { return false; }
bool BTNodeGraphEditor::LoadFromJson(const std::string&) { return false; }

void BTNodeGraphEditor::DrawMenuBar() {}
void BTNodeGraphEditor::DrawNodeGraph() {}
void BTNodeGraphEditor::DrawSidePanel() {}
void BTNodeGraphEditor::DrawNodeContextMenu() {}
void BTNodeGraphEditor::DetectContextMenu() {}
void BTNodeGraphEditor::DrawContextMenuPopup() {}
int  BTNodeGraphEditor::AddNode(BTEditor::NodeKind, float, float) { return -1; }
bool BTNodeGraphEditor::AddLink(int, int) { return false; }
void BTNodeGraphEditor::DeleteNode(int) {}
void BTNodeGraphEditor::DeleteLink(int) {}
void BTNodeGraphEditor::DeleteSelectedNodes() {}
void BTNodeGraphEditor::DeleteSelectedLinks() {}
void BTNodeGraphEditor::DrawParamEditor(BTEditor::EditorNode&) {}
void BTNodeGraphEditor::DrawChargeDashParamEditor(BTEditor::ChargeDashParamData&) {}
unsigned int BTNodeGraphEditor::NodeColor(BTEditor::NodeKind)      const { return 0; }
unsigned int BTNodeGraphEditor::NodeTitleColor(BTEditor::NodeKind) const { return 0; }
bool BTNodeGraphEditor::WouldCreateCycle(int, int) const { return false; }
void BTNodeGraphEditor::ResetToDefault() {}
void BTNodeGraphEditor::AutoLayout() {}
void BTNodeGraphEditor::AutoLayoutHorizontal() {}
void BTNodeGraphEditor::AutoGrouping() {}
void BTNodeGraphEditor::GroupLayout() {}
void BTNodeGraphEditor::DrawGroupPanel() {}
void BTNodeGraphEditor::AddGroup(const std::string&, int, int, int) {}
void BTNodeGraphEditor::DeleteGroup(int) {}
void BTNodeGraphEditor::FocusGroup(int) {}
int  BTNodeGraphEditor::GroupOfNode(int) const { return -1; }

#endif // USE_IMGUI
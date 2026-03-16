#include "BTNodeGraphEditor.h"

#include <imguiManager.h>
#ifdef USE_IMGUI
#include "externals/imgui/imnodes.h"
#endif
#include <json.hpp>

#include <fstream>
#include <algorithm>
#include <cassert>

using namespace BTEditor;

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
	// コンテキストは ImGuiManager が管理するため独自生成しない

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
	const float sidePanelWidth = 300.0f;
	ImGui::BeginChild("##SidePanel",
		ImVec2(sidePanelWidth, 0.0f),
		true,
		ImGuiWindowFlags_None);
	DrawSidePanel();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##GraphCanvas",
		ImVec2(0.0f, 0.0f),
		false,
		ImGuiWindowFlags_None);
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

	// ゲームへ適用ボタン
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
	ImNodes::BeginNodeEditor();

	// ===== ノードの描画 =====
	for (auto& node : graph_.nodes)
	{
		// ノード色を種別で変える
		ImNodes::PushColorStyle(ImNodesCol_NodeBackground, NodeColor(node.kind));
		ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered,
			NodeColor(node.kind) + 0x00101010u);
		ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected,
			NodeColor(node.kind) + 0x00202020u);
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, NodeTitleColor(node.kind));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, NodeTitleColor(node.kind) + 0x00101010u);
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, NodeTitleColor(node.kind) + 0x00202020u);

		ImNodes::BeginNode(node.ImNodeId());

		// --- タイトルバー ---
		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted(node.label.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("[%s]", NodeKindName(node.kind));
		ImNodes::EndNodeTitleBar();

		// --- 入力ピン（Root 以外） ---
		if (node.kind != NodeKind::Root) {
			ImNodes::BeginInputAttribute(node.InputPinId());
			ImGui::TextDisabled("in");
			ImNodes::EndInputAttribute();
		}

		// --- ノード本体コンテンツ（簡易サマリ表示） ---
		if (node.kind == NodeKind::Leaf) {
			ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f),
				"%s", LeafStateTypeName(node.param.stateType));
		}

		// --- 出力ピン（Leaf 以外） ---
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

	// ===== 右クリックコンテキストメニュー =====
	DrawNodeContextMenu();

	ImNodes::EndNodeEditor();

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
void BTNodeGraphEditor::DrawNodeContextMenu()
{
	// 空白部分を右クリックでメニュー
	int hoveredNodeCtx_ = -1, hoveredLinkCtx_ = -1;
	if (ImNodes::IsEditorHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
		!ImNodes::IsNodeHovered(&hoveredNodeCtx_) &&
		!ImNodes::IsLinkHovered(&hoveredLinkCtx_))
	{
		contextMenuOpen_ = true;
		ImVec2 mp = ImGui::GetMousePos();
		contextMenuPosX_ = mp.x;
		contextMenuPosY_ = mp.y;
		ImGui::OpenPopup("##CanvasContextMenu");
	}

	if (ImGui::BeginPopup("##CanvasContextMenu"))
	{
		ImGui::TextDisabled("ノードを追加");
		ImGui::Separator();

		// スクリーン座標 → グリッド座標に変換
		// EditorContextGetPanning() は全バージョンで使える
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 panning = ImNodes::EditorContextGetPanning();
		float gx = (contextMenuPosX_ - winPos.x) - panning.x;
		float gy = (contextMenuPosY_ - winPos.y) - panning.y;

		auto AddAt = [&](NodeKind k) {
			int id = AddNode(k, gx, gy);
			(void)id;
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
// サイドパネル（選択ノードのパラメーター編集）
//======================================================
void BTNodeGraphEditor::DrawSidePanel()
{
	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "BT Node Editor");
	ImGui::Separator();

	// グラフ統計
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
			"ChargeDash",
			"SummonMinion",
			"FindTarget",
			"ChaseTarget",
			"NearIdle",
			"StayHome",
			"IsTargetFar",
			"Custom",
		};
		int cur = static_cast<int>(node.param.stateType);
		if (ImGui::Combo("アクション種別", &cur, kStateNames, IM_ARRAYSIZE(kStateNames))) {
			node.param.stateType = static_cast<LeafStateType>(cur);
			// ラベルを自動更新
			node.label = kStateNames[cur];
		}

		ImGui::Spacing();

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
// デフォルトグラフ（Root ノード1個だけ）
//======================================================
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
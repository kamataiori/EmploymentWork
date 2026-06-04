#pragma once
#include "BTEditorData.h"
#include <string>
#include <functional>
#include <unordered_map>

//======================================================
// BTNodeGraphEditor.h
//
// imnodes を使った UE5 風ビヘイビアツリーノードエディター。
// 以下を担当:
//   - ノードの描画 (Draw)
//   - ノードの追加・削除・接続
//   - ノード内パラメーター編集 (ImGui)
//   - JSON 保存 / 読み込み
//
// BTEditorScene から毎フレーム Draw() を呼ぶだけで動く。
//======================================================
class BTNodeGraphEditor
{
public:
	BTNodeGraphEditor();
	~BTNodeGraphEditor();

	void Initialize();

	void Finalize();

	// 毎フレーム呼ぶ（ImGui ウィンドウごと描画）
	void Draw();

	// ツリーを JSON ファイルへ保存
	bool SaveToJson(const std::string& filepath);

	// JSON ファイルからツリーを読み込み
	bool LoadFromJson(const std::string& filepath);

	// 現在のグラフへの参照（ゲーム側がランタイム BTを構築する時に使う）
	const BTEditor::BTGraph& GetGraph() const { return graph_; }

	// ゲーム適用コールバック（保存ボタン押下時に外部から登録）
	using ApplyCallback = std::function<void(const BTEditor::BTGraph&)>;
	void SetApplyCallback(ApplyCallback cb) { applyCallback_ = std::move(cb); }

private:
#ifdef USE_IMGUI
	//--------------------------------------------------
	// 描画サブルーチン（USE_IMGUI 時のみ）
	//--------------------------------------------------
	void DrawMenuBar();
	void DrawNodeGraph();
	void DrawSidePanel();
	void DrawNodeContextMenu();   // 旧（未使用）
	void DetectContextMenu();     // BeginNodeEditor 内：右クリック検出
	void DrawContextMenuPopup();  // EndNodeEditor 後：ポップアップ描画

	//--------------------------------------------------
	// ノード操作（USE_IMGUI 時のみ）
	//--------------------------------------------------
	// 新しいノードを追加してIDを返す
	int  AddNode(BTEditor::NodeKind kind, float posX = 200.0f, float posY = 200.0f);
	// リンクを追加（循環・多重リンクチェック付き）
	bool AddLink(int fromNodeId, int toNodeId);
	// ノードを削除（関連リンクも削除）
	void DeleteNode(int nodeId);
	// リンクを削除
	void DeleteLink(int linkId);
	// 選択ノードを全削除
	void DeleteSelectedNodes();
	// 選択リンクを全削除
	void DeleteSelectedLinks();

	//--------------------------------------------------
	// パラメーター編集 UI（USE_IMGUI 時のみ）
	//--------------------------------------------------
	void DrawParamEditor(BTEditor::EditorNode& node);
	void DrawChargeDashParamEditor(BTEditor::ChargeDashParamData& p);

	//--------------------------------------------------
	// ユーティリティ（USE_IMGUI 時のみ）
	//--------------------------------------------------
	// ノードの色（種別ごと）
	unsigned int NodeColor(BTEditor::NodeKind kind) const;
	unsigned int NodeTitleColor(BTEditor::NodeKind kind) const;

	// ノードを階層ごとに自動整列
	void AutoLayout();             // 上→下（縦方向）
	void AutoLayoutHorizontal();   // 左→右（横方向）
	void AutoGrouping(); // ツリー解析で自動グループ分け
	void GroupLayout();  // グループ単位で縦にまとめて配置

	// グループ管理
	void DrawGroupPanel();              // グループ一覧UI
	void AddGroup(const std::string& name, int r, int g, int b);
	void DeleteGroup(int groupId);
	void FocusGroup(int groupId);       // グループにカメラを移動
	int  GroupOfNode(int nodeId) const; // ノードが属するグループIDを返す

	// USE_IMGUI 時のみ必要なメンバ変数
	int    selectedNodeId_ = -1;
	int    selectedGroupId_ = -1;  // グループパネルで選択中
	float  zoomScale_ = 1.0f; // ズームスケール（0.3〜2.5）
	bool   contextMenuOpen_ = false;
	float  contextMenuPosX_ = 0.0f;
	float  contextMenuPosY_ = 0.0f;
#endif // USE_IMGUI

	//--------------------------------------------------
	// ユーティリティ（常に有効）
	//--------------------------------------------------
	// 循環チェック: toNodeId が fromNodeId の祖先か
	bool WouldCreateCycle(int fromNodeId, int toNodeId) const;

	// グラフをデフォルト（Rootのみ）に初期化
	void ResetToDefault();

	//--------------------------------------------------
	// 状態（常に有効）
	//--------------------------------------------------
	BTEditor::BTGraph   graph_;
	std::string         saveFilePath_ = "Resources/BT/Enemy/enemy_bt.json";

	ApplyCallback applyCallback_;
};
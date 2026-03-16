#pragma once
#include "BTEditorData.h"
#include <string>
#include <functional>

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

	// 初期化（imnodes コンテキスト生成）
	void Initialize();

	// 後処理（imnodes コンテキスト破棄）
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
	//--------------------------------------------------
	// 描画サブルーチン
	//--------------------------------------------------
	void DrawMenuBar();
	void DrawNodeGraph();
	void DrawSidePanel();
	void DrawNodeContextMenu();

	//--------------------------------------------------
	// ノード操作
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
	// パラメーター編集 UI（サイドパネル内）
	//--------------------------------------------------
	void DrawParamEditor(BTEditor::EditorNode& node);
	void DrawChargeDashParamEditor(BTEditor::ChargeDashParamData& p);

	//--------------------------------------------------
	// ユーティリティ
	//--------------------------------------------------
	// ノードの色（種別ごと）
	unsigned int NodeColor(BTEditor::NodeKind kind) const;
	unsigned int NodeTitleColor(BTEditor::NodeKind kind) const;

	// 循環チェック: toNodeId が fromNodeId の祖先か
	bool WouldCreateCycle(int fromNodeId, int toNodeId) const;

	// グラフをデフォルト（Rootのみ）に初期化
	void ResetToDefault();

	//--------------------------------------------------
	// 状態
	//--------------------------------------------------
	BTEditor::BTGraph   graph_;
	int                 selectedNodeId_ = -1;   // サイドパネルで編集中のノード
	std::string         saveFilePath_ = "Resources/BT/Enemy/enemy_bt.json";

	// 右クリックメニュー用
	bool   contextMenuOpen_ = false;
	float  contextMenuPosX_ = 0.0f;
	float  contextMenuPosY_ = 0.0f;

	ApplyCallback applyCallback_;
};
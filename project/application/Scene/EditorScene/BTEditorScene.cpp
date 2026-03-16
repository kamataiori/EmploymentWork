#include "BTEditorScene.h"

#include "SceneManager.h"
#include "imguiManager.h"

// BT ノード
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"
#include "application/AI/BehaviorTree/Nodes/Decorator/InverterDecorator.h"
// Leaves
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/ChaseTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/StayHomeLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsTargetFarLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/ExecuteStateLeaf.h"
// States
#include "application/Character/CharacterBase/Enemy/State/ChargeDashState.h"
#include "application/Character/CharacterBase/Enemy/State/SummonMinionState.h"

#include <functional>
#include <stdexcept>

using namespace BTEditor;

//======================================================
// Initialize
//======================================================
void BTEditorScene::Initialize()
{
	editor_.Initialize();

	// BT 適用コールバックを登録
	editor_.SetApplyCallback([this](const BTGraph& g) {
		ApplyGraphToEnemy(g);
		});

	// デフォルト JSON を自動ロード（存在すれば）
	editor_.LoadFromJson("Resources/BT/Enemy/enemy_bt.json");
}

//======================================================
// Finalize
//======================================================
void BTEditorScene::Finalize()
{
	editor_.Finalize();
}

//======================================================
// Update
//======================================================
void BTEditorScene::Update()
{
	req.type = TransitionType::Shutter;
	req.fadeOutSec = 2.0f;
	req.fadeInSec = 2.5f;

	// Esc でゲームシーンへ戻る
	if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}



	Debug();
}

//======================================================
// Draw（ImGui 全描画）
//======================================================
void BTEditorScene::Draw()
{
	
}

void BTEditorScene::Debug()
{
#ifdef _DEBUG
	if (!IsDockedImGuiEnabled()) return;
	// 戻るボタン（画面左上に固定オーバーレイ）
	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(160, 40), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.75f);
	ImGui::Begin("##BackBtn",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	if (ImGui::Button("< Back to Game", ImVec2(145, 28))) {
		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}
	ImGui::End();

	// グラフエディター本体
	editor_.Draw();
#endif
}

//======================================================
// ApplyGraphToEnemy
// BTGraph → ランタイム INode ツリーを再構築して
// EnemyAIController に差し込む
//======================================================
void BTEditorScene::ApplyGraphToEnemy(const BTGraph& graph)
{
	if (!aiController_) {
		// aiController_ が未設定の場合は JSON 保存だけ行い、
		// ゲーム起動時に読み込ませる運用も可能
		editor_.SaveToJson("Resources/BT/Enemy/enemy_bt.json");
		return;
	}

	// EnemyAIController に RebuildFromGraph を追加している前提で呼ぶ
	// (追加方法は下記の「EnemyAIController への追加コード」を参照)
	aiController_->RebuildFromGraph(graph);

	// JSON にも同期保存
	editor_.SaveToJson("Resources/BT/Enemy/enemy_bt.json");
}
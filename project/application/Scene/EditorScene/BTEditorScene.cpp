#include "BTEditorScene.h"

#include "SceneManager.h"
#ifdef USE_IMGUI
#include "imguiManager.h"
#endif // USE_IMGUI

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
#ifdef USE_IMGUI
	editor_.Initialize();

	// BT 適用コールバックを登録
	editor_.SetApplyCallback([this](const BTGraph& g) {
		ApplyGraphToEnemy(g);
		});

	// デフォルト JSON を自動ロード（存在すれば）
	editor_.LoadFromJson("Resources/BT/Enemy/enemy_bt.json");
#endif // USE_IMGUI
}

//======================================================
// Finalize
//======================================================
void BTEditorScene::Finalize()
{
#ifdef USE_IMGUI
	editor_.Finalize();
#endif // USE_IMGUI
}

//======================================================
// Update
//======================================================
void BTEditorScene::Update()
{
	req.type = TransitionType::Shutter;
	req.fadeOutSec = 2.0f;
	req.fadeInSec = 2.5f;

#ifdef USE_IMGUI
	// Esc でゲームシーンへ戻る
	if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}
#endif // USE_IMGUI
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
#ifdef USE_IMGUI
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
#endif // USE_IMGUI
}

//======================================================
// ApplyGraphToEnemy
//======================================================
void BTEditorScene::ApplyGraphToEnemy(const BTGraph& graph)
{
	if (!aiController_) {
		editor_.SaveToJson("Resources/BT/Enemy/enemy_bt.json");
		return;
	}

	aiController_->RebuildFromGraph(graph);

	editor_.SaveToJson("Resources/BT/Enemy/enemy_bt.json");
}
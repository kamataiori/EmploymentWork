#pragma once
#include "BaseScene.h"
#include "BTNodeGraphEditor.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "application/Character/CharacterBase/Enemy/AI/ChargeDashParam.h"

#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"

//======================================================
// BTEditorScene.h
//
// SceneManager::ChangeScene("EDITOR") で遷移する専用シーン。
// BTNodeGraphEditor を保持し毎フレーム Draw() を呼ぶ。
//
// 「Apply to Game」ボタンが押されたとき:
//   BTGraph → BuildTreeFromGraph() → EnemyAIController へ反映
//
// ゲームシーンには戻れる (Escキー or "Back to Game" ボタン)
//======================================================
class BTEditorScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize()   override;
	void Update()     override;
	void Draw()       override;
	void BackGroundDraw() override {}
	void ForeGroundDraw() override {}
	void Debug() override;

private:
	// JSON グラフ → ランタイム BT を再構築して enemy に適用
	void ApplyGraphToEnemy(const BTEditor::BTGraph& graph);

private:
	TransitionRequest req{};

	BTNodeGraphEditor editor_;

	// ゲームシーンで生成した EnemyAIController へのポインタ
	// (シーン間共有はシングルトンや SceneManager 経由でも可)
	EnemyAIController* aiController_ = nullptr;
};
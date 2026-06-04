#pragma once
#include "EnemyActionState.h"
#include "application/Character/CharacterBase/Enemy/AI/ChargeDashParam.h"
#include "MathFunctions.h"

//======================================================
// ChargeDashState
//------------------------------------------------------
// 元 ChargeDashAttackLeaf の Phase ロジックをそのまま移植
	// Idle(待機) → Charge(溜め) → Dash(突進) → Recover(硬直) → 完了
//======================================================
class ChargeDashState : public EnemyActionState
{
public:
	explicit ChargeDashState(const ChargeDashParam& param);

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "ChargeDash"; }

private:
	enum class Phase {
		Idle,       // homeで待機（プレイヤーを見る）
		Charge,     // 溜め（home固定）
		Dash,       // 突進
		Recover,    // 突進後硬直
	};

	Phase phase_ = Phase::Idle;
	ChargeDashParam param_;

	float timer_ = 0.0f;

	Vector3 dashDir_{ 0.0f, 0.0f, 1.0f };
	float dashTraveled_ = 0.0f;
	Vector3 dashStartPos_{};
};
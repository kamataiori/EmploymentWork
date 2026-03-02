#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include "application/Character/CharacterBase/Enemy/AI/ChargeDashParam.h"

#include "MathFunctions.h"

// Forward
class Enemy;
struct Transform;

//======================================================
// ChargeDashAttackLeaf
//------------------------------------------------------
// ・溜め中：home(初期位置)で停止、ターゲット方向へ向く
// ・溜め完了：方向ロック
// ・突進：ロック方向へ一定距離だけ前進
// ・終了：homeへ戻す、硬直→クールダウン
//
// BlackBoard key:
//   "enemy"  : Enemy*
//   "target" : const Transform*
//======================================================
class ChargeDashAttackLeaf : public LeafNodeBase {
public:
	ChargeDashAttackLeaf(BlackBoard* bb, const std::string& paramKey);

protected:
	void init() override;
	void tick() override;

private:
	enum class Phase {
		Idle,       // homeで待機（プレイヤーを見る）
		Charge,     // 溜め（home固定）
		Dash,       // 突進
		EndStop,    // 突進後その場で停止
		DropShot,
	};

	Phase phase_ = Phase::Idle;

	std::string paramKey_;

	float timer_ = 0.0f;

	Vector3 dashDir_{ 0.0f, 0.0f, 1.0f };   // 突進方向（Y=0）
	float dashTraveled_ = 0.0f;             // どれだけ進んだか
	Vector3 dashStartPos_{};
	Vector3 dashEndPos_{};
	float endStopTime_ = 0.25f;  // ここはParamに入れてもOK
};
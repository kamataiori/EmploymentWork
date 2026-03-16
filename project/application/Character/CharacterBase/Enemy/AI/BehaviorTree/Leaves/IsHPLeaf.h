#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

//======================================================
// IsHPLeaf
//------------------------------------------------------
// Enemy の HP フェーズを判定する Condition Leaf
//
// targetPhase_ 以下のフェーズになったら Success を返す
//
// 使い方:
//   IsHPLeaf(bb, Enemy::EnemyPhase::Phase2)
//   → HP50%以下（Phase2 or Phase3）なら Success
//======================================================
class IsHPLeaf : public LeafNodeBase
{
public:
	using EnemyPhase = int; // int で受け取り比較（循環 include 回避）

	// targetPhase: 1=Phase1, 2=Phase2(HP50%以下), 3=Phase3(HP25%以下)
	IsHPLeaf(BlackBoard* bb, int targetPhase);

protected:
	void init() override;
	void tick() override;

private:
	int targetPhase_ = 2;
};
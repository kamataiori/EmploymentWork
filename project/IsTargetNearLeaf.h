#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

class Enemy;
struct Transform;

//======================================================
// IsTargetNearLeaf
// ・Enemy と Target の距離が「攻撃距離以内か？」を判定する条件Leaf
// ・近い  -> Success（攻撃してよい）
// ・遠い  -> Fail（攻撃しない）
//
// BlackBoard
//   "enemy"  : Enemy*
//   "target" : const Transform*
//======================================================
class IsTargetNearLeaf : public LeafNodeBase {
public:
    IsTargetNearLeaf(BlackBoard* bb, float attackDist)
        : LeafNodeBase(bb), attackDist_(attackDist) {
    }

protected:
    void init() override { NodeBase::init(); }

    void tick() override;

private:
    float attackDist_ = 3.0f; // 攻撃開始距離
};

#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include "Enemy/Enemy.h"

//======================================================
// NearIdleLeaf
// ・近距離時は「停止」させるだけ
// ・将来ここを AttackLeaf に差し替えると自然に拡張できる
//
// BlackBoard から読む：
//   "enemy" : Enemy*
//======================================================
class NearIdleLeaf : public LeafNodeBase {
public:
    explicit NearIdleLeaf(BlackBoard* bb) : LeafNodeBase(bb) {}

protected:
    void init() override { NodeBase::init(); }

    void tick() override
    {
        Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
        if (enemy) {
            enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
        }
        mNodeResult = NodeResult::Success;
    }
};

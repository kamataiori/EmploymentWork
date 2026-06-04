#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

// Forward
class Enemy;
struct Transform;

//======================================================
// StayHomeLeaf
//------------------------------------------------------
// ・敵を「初期位置(homePosition)」に固定する（追従しない）
// ・ターゲット方向へだけ向く（Yaw回転のみ）
// ・攻撃ノードの前に置いて「位置だけは常に初期位置」を保証する
//
// BlackBoard から読むキー：
//   "enemy"  : Enemy*
//   "target" : const Transform*（無くてもOK）
//======================================================
class StayHomeLeaf : public LeafNodeBase {
public:
    // turnLerp : 旋回の滑らかさ（0に近いほどゆっくり、1で即時）
    StayHomeLeaf(BlackBoard* bb, float turnLerp);

protected:
    void init() override;
    void tick() override;

private:
    float turnLerp_ = 0.2f;
};
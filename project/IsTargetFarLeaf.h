#pragma once
#include "LeafNodeBase.h"
#include "BlackBoard.h"

// 前方宣言
class Enemy;
struct Transform;

//======================================================
// IsTargetFarLeaf
// ・Enemy と Target の距離が「追跡開始距離より遠いか？」を判定
// ・遠いなら Success / 近いなら Fail
//
// BlackBoard から読む：
//   "enemy"  : Enemy*
//   "target" : const Transform*
//======================================================
class IsTargetFarLeaf : public LeafNodeBase {
public:
    IsTargetFarLeaf(BlackBoard* bb, float chaseStartDist);

protected:
    void init() override;
    void tick() override;

private:
    float chaseStartDist_ = 12.0f;
};

#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include <functional>

// 前方宣言
struct Transform;

//======================================================
// FindTargetLeaf
// ・Player(ターゲット)を探して BlackBoard に保存する
// ・成功したら Success / 失敗したら Fail
//
// BlackBoard に入れるもの：
//   "target" : const Transform*
//======================================================
class FindTargetLeaf : public LeafNodeBase {
public:
    using TargetGetter = std::function<const Transform* ()>;

    FindTargetLeaf(BlackBoard* bb, TargetGetter getter);

protected:
    void init() override;
    void tick() override;

private:
    TargetGetter getter_;
};

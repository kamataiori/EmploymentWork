#include "FindTargetLeaf.h"

FindTargetLeaf::FindTargetLeaf(BlackBoard* bb, TargetGetter getter)
    : LeafNodeBase(bb), getter_(std::move(getter)) {
}

void FindTargetLeaf::init()
{
    NodeBase::init();
}

void FindTargetLeaf::tick()
{
    const Transform* t = nullptr;

    // Scene側から注入された getter があれば使用
    if (getter_) {
        t = getter_();
    }

    // 見つからない
    if (!t) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    // BlackBoardへ保存（後続ノードが参照）
    mpBlackBoard->set_value<const Transform*>("target", t);

    mNodeResult = NodeResult::Success;
}

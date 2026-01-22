#include "BranchNodeBase.h"

BranchNodeBase::BranchNodeBase(BlackBoard* bb)
    : NodeBase(bb) {
}

BranchNodeBase::~BranchNodeBase() = default;

void BranchNodeBase::set_true_node(std::unique_ptr<INode> node)
{
    mpBranchNodes[0] = std::move(node);
}

void BranchNodeBase::set_false_node(std::unique_ptr<INode> node)
{
    mpBranchNodes[1] = std::move(node);
}

void BranchNodeBase::init()
{
    NodeBase::init();
    mSatisfyIndex = is_condition() ? 0 : 1;
}

void BranchNodeBase::tick()
{
    auto* child = mpBranchNodes[mSatisfyIndex].get();
    if (!child) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    child->execute();
    mNodeResult = child->get_node_result();
}

void BranchNodeBase::finalize()
{
    // 両方Reset（次回分岐が変わっても大丈夫にする）
    for (int i = 0; i < 2; ++i) {
        if (!mpBranchNodes[i]) continue;
        mpBranchNodes[i]->Reset();
    }

    // 分岐結果も初期化
    mSatisfyIndex = -1;

    NodeBase::finalize();
}

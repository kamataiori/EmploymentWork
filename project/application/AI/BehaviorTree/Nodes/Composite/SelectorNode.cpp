#include "SelectorNode.h"

SelectorNode::SelectorNode(BlackBoard* bb)
    : CompositeNodeBase(bb) {
}

void SelectorNode::init()
{
    NodeBase::init();
    mRunningNodeIndex = 0;
}

void SelectorNode::tick()
{
    if (mChildNodes.empty()) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    auto& child = mChildNodes[mRunningNodeIndex];
    child->execute();

    auto r = child->get_node_result();
    if (r == NodeResult::Running) {
        mNodeResult = NodeResult::Running;
    }
    else if (r == NodeResult::Success) {
        mNodeResult = NodeResult::Success;
    }
    else {
        mRunningNodeIndex = get_next_index();
        if (mRunningNodeIndex >= (int)mChildNodes.size()) {
            mNodeResult = NodeResult::Fail;
        }
        else {
            mNodeResult = NodeResult::Running;
        }
    }
}

int SelectorNode::get_next_index() const
{
    return mRunningNodeIndex + 1;
}

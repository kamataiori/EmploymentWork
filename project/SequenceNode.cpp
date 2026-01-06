#include "SequenceNode.h"

SequenceNode::SequenceNode(BlackBoard* bb)
    : CompositeNodeBase(bb) {
}

void SequenceNode::init()
{
    NodeBase::init();
    mRunningNodeIndex = 0;
}

void SequenceNode::tick()
{
    if (mChildNodes.empty()) {
        mNodeResult = NodeResult::Success;
        return;
    }

    auto& child = mChildNodes[mRunningNodeIndex];
    child->execute();

    auto r = child->get_node_result();
    if (r == NodeResult::Running) {
        mNodeResult = NodeResult::Running;
    }
    else if (r == NodeResult::Fail) {
        mNodeResult = NodeResult::Fail;
    }
    else {
        mRunningNodeIndex = get_next_index();
        if (mRunningNodeIndex >= (int)mChildNodes.size()) {
            mNodeResult = NodeResult::Success;
        }
        else {
            mNodeResult = NodeResult::Running;
        }
    }
}

int SequenceNode::get_next_index() const
{
    return mRunningNodeIndex + 1;
}

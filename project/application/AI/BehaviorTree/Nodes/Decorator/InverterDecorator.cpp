#include "InverterDecorator.h"

InverterDecorator::InverterDecorator(BlackBoard* bb)
    : DecoratorNodeBase(bb) {
}

void InverterDecorator::tick()
{
    if (!mChildNode) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    mChildNode->execute();
    auto r = mChildNode->get_node_result();

    if (r == NodeResult::Running) {
        mNodeResult = NodeResult::Running;
    }
    else if (r == NodeResult::Success) {
        mNodeResult = NodeResult::Fail;
    }
    else if (r == NodeResult::Fail) {
        mNodeResult = NodeResult::Success;
    }
}

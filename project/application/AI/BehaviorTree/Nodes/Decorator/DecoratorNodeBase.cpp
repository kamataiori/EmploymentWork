#include "DecoratorNodeBase.h"

DecoratorNodeBase::DecoratorNodeBase(BlackBoard* bb)
    : NodeBase(bb) {
}

DecoratorNodeBase::~DecoratorNodeBase() = default;

void DecoratorNodeBase::set_node(std::unique_ptr<INode> node)
{
    mChildNode = std::move(node);
}

void DecoratorNodeBase::finalize()
{
    // 子をReset（Cooldown/Wait等の内部状態が残らないように）
    if (mChildNode) {
        if (auto* nb = dynamic_cast<NodeBase*>(mChildNode.get())) {
            nb->Reset();
        }
    }

    NodeBase::finalize();
}

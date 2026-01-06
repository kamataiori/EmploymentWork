#include "CompositeNodeBase.h"

CompositeNodeBase::CompositeNodeBase(BlackBoard* bb)
    : NodeBase(bb) {
}

CompositeNodeBase::~CompositeNodeBase() = default;

void CompositeNodeBase::add_node(std::unique_ptr<INode> node)
{
    mChildNodes.emplace_back(std::move(node));
}

void CompositeNodeBase::finalize()
{
    // 子が内部状態を持っていても、次回正常に動くようReset
    for (auto& c : mChildNodes) {
        if (!c) continue;

        // NodeBase派生ならResetを呼べる
        if (auto* nb = dynamic_cast<NodeBase*>(c.get())) {
            nb->Reset();
        }
    }

    // RunningIndexも初期化
    mRunningNodeIndex = 0;

    NodeBase::finalize();
}

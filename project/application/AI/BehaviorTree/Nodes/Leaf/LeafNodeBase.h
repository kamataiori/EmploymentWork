#pragma once
#include "application/AI/BehaviorTree/Core/NodeBase.h"

//======================================================
// LeafNodeBase
// 実処理担当ノード
//======================================================
class LeafNodeBase : public NodeBase {
protected:
    explicit LeafNodeBase(BlackBoard* bb);

public:
    virtual ~LeafNodeBase();
};

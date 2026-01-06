#pragma once
#include "NodeBase.h"

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

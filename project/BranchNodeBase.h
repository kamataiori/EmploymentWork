#pragma once
#include <memory>
#include "NodeBase.h"

//======================================================
// BranchNodeBase
// ・条件分岐ノード（If）
// ・finalize() で両子をResetし、mSatisfyIndexも初期化
//======================================================
class BranchNodeBase : public NodeBase {
protected:
    explicit BranchNodeBase(BlackBoard* bb);

public:
    virtual ~BranchNodeBase();

    void set_true_node(std::unique_ptr<INode> node);
    void set_false_node(std::unique_ptr<INode> node);

protected:
    virtual bool is_condition() = 0;

    void init() override;
    void tick() override;
    void finalize() override;

protected:
    std::unique_ptr<INode> mpBranchNodes[2];
    int mSatisfyIndex = -1;
};

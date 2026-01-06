#pragma once
#include "INode.h"

class BlackBoard;

//======================================================
// NodeBase
// init / tick / finalize の流れを一元管理
//======================================================
class NodeBase : public INode {
protected:
    explicit NodeBase(BlackBoard* black_board);

public:
    virtual ~NodeBase();

    // BT実行の入口
    void execute() override;
    // BT実行の入口
    NodeResult get_node_result() const override;

    // このノードを未実行状態へ戻す
    //  - Composite/Decorator/Branch が子ノードを安全にリセットできる
    virtual void Reset();

protected:
    // デフォルト挙動（派生で override）
    void init() override;
    void tick() override;
    void finalize() override;

protected:
    NodeResult mNodeResult = NodeResult::Idle;
    BlackBoard* mpBlackBoard = nullptr;
};

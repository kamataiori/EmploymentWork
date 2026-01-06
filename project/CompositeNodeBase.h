#pragma once
#include <vector>
#include <memory>
#include "NodeBase.h"

//======================================================
// CompositeNodeBase
// ・子ノードを複数持つ（Sequence/Selector）
// ・Running中インデックスを持つ
// ・finalize() で子をResetして「次フレーム再評価」を保証
//======================================================
class CompositeNodeBase : public NodeBase {
protected:
    explicit CompositeNodeBase(BlackBoard* bb);

public:
    virtual ~CompositeNodeBase();

    void add_node(std::unique_ptr<INode> node);

protected:
    // Composite共通の後処理
    // ・子ノードを全Reset
    // ・RunningIndexを0に戻す
    void finalize() override;


    std::vector<std::unique_ptr<INode>> mChildNodes;
    int mRunningNodeIndex = 0;

    virtual int get_next_index() const = 0;
};

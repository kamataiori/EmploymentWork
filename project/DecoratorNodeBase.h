#pragma once
#include <memory>
#include "NodeBase.h"

//======================================================
// DecoratorNodeBase
// ・子ノード1つだけ持つ
// ・finalize() で子をResetして状態残りを防ぐ
//======================================================
class DecoratorNodeBase : public NodeBase {
protected:
    explicit DecoratorNodeBase(BlackBoard* bb);

public:
    virtual ~DecoratorNodeBase();

    void set_node(std::unique_ptr<INode> node);

protected:
    // Decorator共通の後処理
    void finalize() override;

protected:
    std::unique_ptr<INode> mChildNode;
};

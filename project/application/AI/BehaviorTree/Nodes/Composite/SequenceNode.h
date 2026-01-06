#pragma once
#include "CompositeNodeBase.h"

//======================================================
// SequenceNode
//======================================================
class SequenceNode : public CompositeNodeBase {
public:
    explicit SequenceNode(BlackBoard* bb);

protected:
    void init() override;
    void tick() override;
    int get_next_index() const override;
};

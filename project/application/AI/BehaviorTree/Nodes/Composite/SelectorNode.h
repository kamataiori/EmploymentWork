#pragma once
#include "CompositeNodeBase.h"

//======================================================
// SelectorNode
//======================================================
class SelectorNode : public CompositeNodeBase {
public:
    explicit SelectorNode(BlackBoard* bb);

protected:
    void init() override;
    void tick() override;
    int get_next_index() const override;
};

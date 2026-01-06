#pragma once
#include "DecoratorNodeBase.h"

//======================================================
// InverterDecorator
// Success <-> Fail を反転
//======================================================
class InverterDecorator : public DecoratorNodeBase {
public:
    explicit InverterDecorator(BlackBoard* bb);

protected:
    void tick() override;
};

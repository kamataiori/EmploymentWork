#pragma once
#include "EnemyState.h"

class EnemyState_Attack2 : public EnemyState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    const char* GetName() const override { return "Attack2"; }
private:
    float timer_ = 0.0f;
};

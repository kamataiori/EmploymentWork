#pragma once
#include "EnemyState.h"

class EnemyState_Idle : public EnemyState {
public:
    EnemyState_Idle() = default;

    void Enter(Enemy* enemy) override {}
    void Update(Enemy* enemy) override;
    const char* GetName() const override { return "Idle"; }
};

#pragma once
#include "EnemyState.h"

class EnemyState_Dash : public EnemyState {
public:
    EnemyState_Dash() = default;

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    const char* GetName() const override { return "Dash"; }

private:
    Vector3 direction = {};  // ダッシュ方向
    float dashTime = 0.5f;   // ダッシュ持続時間
    float timer = 0.0f;      // 経過時間
};

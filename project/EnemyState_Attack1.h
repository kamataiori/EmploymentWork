#pragma once
#include "EnemyState.h"

class EnemyState_Attack1 : public EnemyState {
public:
    EnemyState_Attack1() = default;

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    const char* GetName() const override { return "Attack1"; }

private:
    // 攻撃後の待機時間
    float timer_ = 0.0f;

    // 弾の速さ
    float bulletSpeed = 1.0f;
};

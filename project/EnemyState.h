#pragma once
#include "MathFunctions.h"
#include <memory>

class Enemy;

class EnemyState {
public:
    virtual ~EnemyState() = default;

    // 状態開始時に呼ばれる
    virtual void Enter(Enemy* enemy) = 0;

    // 毎フレーム呼ばれる
    virtual void Update(Enemy* enemy) = 0;

    // 状態名を返す（ImGui表示用）
    virtual const char* GetName() const = 0;
};

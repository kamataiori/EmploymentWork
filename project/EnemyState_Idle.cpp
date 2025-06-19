#include "EnemyState_Idle.h"
#include "Enemy.h"
#include "EnemyState_Dash.h"
#include <cstdlib>

void EnemyState_Idle::Enter(Enemy* enemy) {
    timer_ = 0.0f;

    // 必要ならランダムな待機時間にしても良い
    // waitTime_ = 1.0f + static_cast<float>(rand() % 100) / 100.0f; // 1.0 ～ 2.0秒
}

void EnemyState_Idle::Update(Enemy* enemy) {
    timer_ += 1.0f / 60.0f;
    if (timer_ >= waitTime_) {
        enemy->ChangeToRandomState();
    }
}

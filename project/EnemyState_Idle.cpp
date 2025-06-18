#include "EnemyState_Idle.h"
#include "Enemy.h"
#include "EnemyState_Dash.h"
#include <cstdlib>

void EnemyState_Idle::Update(Enemy* enemy) {
    static float timer = 0.0f;
    timer += 1.0f / 60.0f;

    if (timer >= 2.0f) {
        timer = 0.0f;

        // Dashに切り替える（Idle→IdleはChangeState側で無視される）
        enemy->ChangeState(std::make_unique<EnemyState_Dash>());
    }
}

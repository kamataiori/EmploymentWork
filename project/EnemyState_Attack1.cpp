#include "EnemyState_Attack1.h"
#include "Enemy.h"
#include "EnemyAttackBullet.h"
#include <cmath>

using namespace DirectX;

void EnemyState_Attack1::Enter(Enemy* enemy) {
    timer_ = 0.0f;

    // プレイヤーへの基準方向
    Vector3 toPlayer = enemy->GetPlayerPos() - enemy->GetTransform().translate;
    toPlayer.y = 0.0f;
    toPlayer = Normalize(toPlayer);

    const float baseAngle = std::atan2(toPlayer.z, toPlayer.x);
    const float spread = XMConvertToRadians(15.0f); // 扇の開き角度

    for (int i = -1; i <= 1; ++i) {
        float angle = baseAngle + i * spread;
        Vector3 dir = {
            std::cos(angle),
            0.0f,
            std::sin(angle)
        };

        auto bullet = std::make_unique<EnemyAttackBullet>(enemy->GetBaseScene());
        bullet->Initialize();
        bullet->SetCamera(enemy->GetCamera());
        bullet->SetTranslate(enemy->GetTransform().translate);
        bullet->SetVelocity(Normalize(dir) * 0.5f);

        enemy->AddBullet(std::move(bullet)); // bullets_ に追加
    }
}

void EnemyState_Attack1::Update(Enemy* enemy) {
    timer_ += 1.0f / 60.0f;

    if (timer_ >= 1.0f) {
        enemy->ChangeToRandomState();
    }
}

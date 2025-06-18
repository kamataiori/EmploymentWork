#include "EnemyState_Attack2.h"
#include "EnemyAreaAttack.h"
#include "Enemy.h"
#include "BaseScene.h"
#include <cmath>

void EnemyState_Attack2::Enter(Enemy* enemy) {
	BaseScene* scene = enemy->GetBaseScene();
	auto areaAttack = std::make_unique<EnemyAreaAttack>(scene);
	
	areaAttack->Initialize();

	// 敵の正面方向を計算
	float angle = enemy->GetTransform().rotate.y;
	Vector3 forward = {
		std::sin(angle),
		0.0f,
		std::cos(angle)
	};
	Vector3 spawnPos = enemy->GetTransform().translate + forward * 5.0f;
	areaAttack->SetTranslate(spawnPos);
	/*areaAttack->SetLifetime(1.0f);*/

	enemy->AddAreaAttack(std::move(areaAttack));
}


void EnemyState_Attack2::Update(Enemy* enemy) {
    // この状態ではその場に立ち止まるだけ
    // 次ステートに遷移するならタイマーなどをここで管理

	timer_ += 1.0f / 60.0f;
	if (timer_ >= 1.0f) {
		enemy->ChangeToRandomState();
	}
}

#include "WaitMinionDeadState.h"
#include "Enemy/Enemy.h"
#include <cmath>

void WaitMinionDeadState::Enter(Enemy* enemy)
{
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}

bool WaitMinionDeadState::Update(Enemy* enemy, float dt)
{
	// プレイヤーの方向をゆっくり向く
	const Transform* target = enemy->GetTargetTransform();
	if (target) {
		Transform e = enemy->GetTransform();
		Vector3 to = target->translate - e.translate;
		to.y = 0.0f;
		if (Length(to) > 1e-6f) {
			float yaw = std::atan2(Normalize(to).x, Normalize(to).z);
			float diff = yaw - e.rotate.y;
			while (diff > 3.14159f) diff -= 6.28318f;
			while (diff < -3.14159f) diff += 6.28318f;
			e.rotate.y += diff * 0.05f;
			enemy->SetTransform(e);
		}
	}

	// 雑魚敵が全滅したら完了
	if (enemy->GetMinions().empty()) {
		return false;
	}

	// 全滅まで永遠に待機
	return true;
}

void WaitMinionDeadState::Exit(Enemy* enemy)
{
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}
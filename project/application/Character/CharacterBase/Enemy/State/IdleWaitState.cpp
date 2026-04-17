#include "IdleWaitState.h"
#include "Enemy/Enemy.h"
#include <cmath>

IdleWaitState::IdleWaitState(float waitTime)
	: waitTime_(waitTime)
{
}

void IdleWaitState::Enter(Enemy* enemy)
{
	timer_ = 0.0f;

	// Idle アニメーション
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}

bool IdleWaitState::Update(Enemy* enemy, float dt)
{
	// プレイヤーの方向だけ向く（移動しない）
	const Transform* target = enemy->GetTargetTransform();
	if (target) {
		Transform e = enemy->GetTransform();
		Vector3 toTarget = target->translate - e.translate;
		toTarget.y = 0.0f;
		if (Length(toTarget) > 1e-6f) {
			Vector3 dir = Normalize(toTarget);
			float desiredYaw = std::atan2(dir.x, dir.z);
			// ゆっくり向く（隙っぽさを演出）
			float diff = desiredYaw - e.rotate.y;
			while (diff > 3.14159f) diff -= 6.28318f;
			while (diff < -3.14159f) diff += 6.28318f;
			e.rotate.y += diff * 0.05f;
			enemy->SetTransform(e);
		}
	}

	timer_ += dt;
	if (timer_ >= waitTime_) {
		return false; // 完了
	}
	return true; // 待機中
}

void IdleWaitState::Exit(Enemy* enemy)
{
	// 特に後処理なし
}
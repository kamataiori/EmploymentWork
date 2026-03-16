#include "ThrowBigBulletState.h"
#include "Enemy/Enemy.h"
#include <cmath>

void ThrowBigBulletState::Enter(Enemy* enemy)
{
	phase_ = Phase::WindUp;
	timer_ = 0.0f;
}

bool ThrowBigBulletState::Update(Enemy* enemy, float dt)
{
	const Transform* target = enemy->GetTargetTransform();
	Transform e = enemy->GetTransform();

	switch (phase_)
	{
	case Phase::WindUp:
	{
		// プレイヤーの方向を向く
		if (target) {
			Vector3 toTarget = target->translate - e.translate;
			toTarget.y = 0.0f;
			if (Length(toTarget) > 1e-6f) {
				Vector3 dir = Normalize(toTarget);
				float desiredYaw = std::atan2(dir.x, dir.z);
				e.rotate.y = desiredYaw;
				enemy->SetTransform(e);
			}
		}

		// 予備動作アニメーション
		if (!enemy->IsHitReact()) {
			enemy->SetAnimationIfChanged(enemy->GetAnimSet().Punch);
		}

		timer_ += dt;
		if (timer_ >= windUpTime_) {
			timer_ = 0.0f;
			phase_ = Phase::Throw;
		}
		break;
	}

	case Phase::Throw:
	{
		// 大弾を発射
		if (target) {
			enemy->SpawnBigBullet(target->translate);
		}

		timer_ = 0.0f;
		phase_ = Phase::Recover;
		break;
	}

	case Phase::Recover:
	{
		// 硬直（プレイヤーの反撃チャンス）
		if (!enemy->IsHitReact()) {
			enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
		}

		timer_ += dt;
		if (timer_ >= recoverTime_) {
			return false; // 完了
		}
		break;
	}
	}

	return true;
}

void ThrowBigBulletState::Exit(Enemy* enemy)
{
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}
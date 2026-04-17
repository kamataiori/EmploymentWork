
#include "ShootSplitBulletState.h"
#include "Enemy/Enemy.h"
#include <cmath>

ShootSplitBulletState::ShootSplitBulletState(bool angryMode)
	: angryMode_(angryMode)
{
}

void ShootSplitBulletState::Enter(Enemy* enemy)
{
	phase_ = Phase::WindUp;
	timer_ = 0.0f;
}

bool ShootSplitBulletState::Update(Enemy* enemy, float dt)
{
	const Transform* target = enemy->GetTargetTransform();
	Transform e = enemy->GetTransform();

	switch (phase_)
	{
	case Phase::WindUp:
	{
		// プレイヤーの方向を向きながら溜める
		if (target)
		{
			Vector3 toTarget = target->translate - e.translate;
			toTarget.y = 0.0f;
			if (Length(toTarget) > 1e-6f)
			{
				Vector3 dir = Normalize(toTarget);
				float desiredYaw = std::atan2(dir.x, dir.z);
				// 素早く向く
				e.rotate.y = desiredYaw;
				enemy->SetTransform(e);
			}
		}

		// Wave アニメーション（発射モーション）
		if (!enemy->IsHitReact()) {
			enemy->SetAnimationIfChanged(enemy->GetAnimSet().Wave);
		}

		timer_ += dt;
		if (timer_ >= windUpTime_)
		{
			timer_ = 0.0f;
			phase_ = Phase::Shoot;
		}
		break;
	}

	case Phase::Shoot:
	{
		// 怒り時は多発（8発）、通常は4発
		if (target) {
			if (angryMode_) {
				enemy->SpawnSplitBurstAngry(target->translate);
			}
			else {
				enemy->SpawnSplitBurstToPlayer(target->translate);
			}
		}

		timer_ = 0.0f;
		phase_ = Phase::Recover;
		break;
	}

	case Phase::Recover:
	{
		timer_ += dt;
		if (timer_ >= recoverTime_)
		{
			// 硬直完了 → ステート終了
			return false;
		}
		break;
	}
	}

	return true; // まだ実行中
}

void ShootSplitBulletState::Exit(Enemy* enemy)
{
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}
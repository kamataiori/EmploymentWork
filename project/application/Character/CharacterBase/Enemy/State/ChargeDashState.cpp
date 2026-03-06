#include "ChargeDashState.h"
#include "Enemy/Enemy.h"
#include "engine/TimeManager.h"
#include <cmath>

// 角度差分を [-pi, pi] に折りたたむ
static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}
// 角度補間（ラップ考慮）
static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

ChargeDashState::ChargeDashState(const ChargeDashParam& param)
	: param_(param)
{
}

void ChargeDashState::Enter(Enemy* enemy)
{
	phase_ = Phase::Idle;
	timer_ = 0.0f;
	dashTraveled_ = 0.0f;
	dashDir_ = { 0.0f, 0.0f, 1.0f };
}

bool ChargeDashState::Update(Enemy* enemy, float dt)
{
	const Transform* target = enemy->GetTargetTransform();
	Transform e = enemy->GetTransform();
	const Vector3 home = enemy->GetHomePosition();

	Vector3 to{ 0,0,0 };
	float dist = 0.0f;
	if (target) {
		to = target->translate - home;
		to.y = 0.0f;
		dist = Length(to);
	}

	auto FaceTargetYaw = [&]() {
		if (target && dist > 1e-6f) {
			Vector3 dir = Normalize(to);
			float desiredYaw = std::atan2(dir.x, dir.z);
			e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, param_.turnLerp);
		}
		};

	switch (phase_)
	{
	case Phase::Idle:
	{
		FaceTargetYaw();

		timer_ += dt;
		if (timer_ >= param_.cooldownTime)
		{
			timer_ = 0.0f;
			phase_ = Phase::Charge;
		}
		break;
	}

	case Phase::Charge:
	{
		FaceTargetYaw();

		timer_ += dt;
		if (timer_ >= param_.chargeTime)
		{
			timer_ = 0.0f;

			Vector3 toTarget = target->translate - e.translate;
			toTarget.y = 0.0f;

			if (Length(toTarget) > 1e-6f)
				dashDir_ = Normalize(toTarget);

			dashStartPos_ = e.translate;
			dashTraveled_ = 0.0f;

			if (!enemy->IsHitReact()) {
				enemy->SetAnimationIfChanged(enemy->GetAnimSet().Run);
			}
			phase_ = Phase::Dash;
		}
		break;
	}

	case Phase::Dash:
	{
		float step = param_.dashSpeed * dt;
		float remain = param_.dashDistance - dashTraveled_;
		if (step > remain) step = remain;

		dashTraveled_ += step;

		e.translate = dashStartPos_ + dashDir_ * dashTraveled_;

		if (Length(dashDir_) > 1e-6f)
		{
			e.rotate.y = std::atan2(dashDir_.x, dashDir_.z);
		}

		if (dashTraveled_ >= param_.dashDistance - 1e-4f)
		{
			timer_ = 0.0f;
			phase_ = Phase::EndStop;
			if (!enemy->IsHitReact()) {
				enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
			}
		}
		break;
	}

	case Phase::EndStop:
	{
		FaceTargetYaw();

		timer_ += dt;
		if (timer_ >= endStopTime_)
		{
			timer_ = 0.0f;
			phase_ = Phase::DropShot;

			if (!enemy->IsHitReact()) {
				enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
			}
		}
		break;
	}

	case Phase::DropShot:
	{
		if (target) {
			enemy->SpawnSplitBurstToPlayer(target->translate);
		}

		// このステートは完了 → false を返す
		enemy->SetTransform(e);
		return false;
	}
	}

	enemy->SetTransform(e);
	return true;  // まだ実行中
}

void ChargeDashState::Exit(Enemy* enemy)
{
	// 必要なら後処理
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}
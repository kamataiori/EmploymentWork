#include "ChargeDashAttackLeaf.h"
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

ChargeDashAttackLeaf::ChargeDashAttackLeaf(BlackBoard* bb, const std::string& paramKey)
	: LeafNodeBase(bb)
	, paramKey_(paramKey)
{
}

void ChargeDashAttackLeaf::init()
{
	NodeBase::init();

	// 最初はすぐ突進しないように、Cooldown開始にしておく
	phase_ = Phase::Idle;
	timer_ = 0.0f;
	dashTraveled_ = 0.0f;
	dashDir_ = { 0.0f, 0.0f, 1.0f };
}

void ChargeDashAttackLeaf::tick()
{
	Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
	if (!enemy) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	// 毎フレーム、黒板から最新パラメータを読む（これがB案の肝）
	ChargeDashParam p{};
	if (mpBlackBoard->try_get_value(paramKey_.c_str(), p) == false) {
		// Paramが入ってないならFail（もしくはデフォルトで続行でもOK）
		mNodeResult = NodeResult::Fail;
		return;
	}

	const Transform* target = mpBlackBoard->get_value<const Transform*>("target");
	const float dt = TimeManager::GetInstance()->GetDeltaTime();

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
			e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, p.turnLerp);
		}
		};

    switch (phase_)
    {
    case Phase::Idle:
    {
        // その場でプレイヤーを見る
        FaceTargetYaw();

        timer_ += dt;
        if (timer_ >= p.cooldownTime)
        {
            timer_ = 0.0f;
            phase_ = Phase::Charge;
        }

        mNodeResult = NodeResult::Running;
        break;
    }

    case Phase::Charge:
    {
        // その場で溜め＋プレイヤーを見る
        FaceTargetYaw();

        timer_ += dt;
        if (timer_ >= p.chargeTime)
        {
            timer_ = 0.0f;

            // 現在位置基準で方向ロック
            Vector3 to = target->translate - e.translate;
            to.y = 0.0f;

            if (Length(to) > 1e-6f)
                dashDir_ = Normalize(to);

            dashStartPos_ = e.translate;
            dashTraveled_ = 0.0f;

            // 被弾中はBTからアニメを触らない
            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Run);
            }
            phase_ = Phase::Dash;
        }

        mNodeResult = NodeResult::Running;
        break;
    }

    case Phase::Dash:
    {
        float step = p.dashSpeed * dt;
        float remain = p.dashDistance - dashTraveled_;
        if (step > remain) step = remain;

        dashTraveled_ += step;

        e.translate = dashStartPos_ + dashDir_ * dashTraveled_;

        // 突進方向を向く
        if (Length(dashDir_) > 1e-6f)
        {
            e.rotate.y = std::atan2(dashDir_.x, dashDir_.z);
        }

        if (dashTraveled_ >= p.dashDistance - 1e-4f)
        {
            timer_ = 0.0f;
            phase_ = Phase::EndStop;
            // Dash終了でIdleに戻す
            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
            }
        }

        mNodeResult = NodeResult::Running;
        break;
    }

    case Phase::EndStop:
    {
        // 突進終了地点で停止
        // 位置は触らない（その場）
        FaceTargetYaw(); // プレイヤーを見る

        timer_ += dt;
        if (timer_ >= endStopTime_)
        {
            timer_ = 0.0f;
            phase_ = Phase::DropShot;

            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
            }
        }

        mNodeResult = NodeResult::Running;
        break;
    }
    case Phase::DropShot:
    {
        // Dash後に1発だけ落とす（プレイヤー位置をロックして狙う）
        if (target) {
            enemy->SpawnSplitBurstToPlayer(target->translate);
        }

        timer_ = 0.0f;
        phase_ = Phase::Idle;

        mNodeResult = NodeResult::Running;
        break;
    }

    }

	enemy->SetTransform(e);
}
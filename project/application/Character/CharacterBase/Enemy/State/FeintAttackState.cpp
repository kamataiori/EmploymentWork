#include "FeintAttackState.h"
#include "Enemy/Enemy.h"
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

FeintAttackState::FeintAttackState(const FeintAttackParam& param)
    : param_(param)
{
}

void FeintAttackState::Enter(Enemy* enemy)
{
    phase_ = Phase::WindUp;
    timer_ = 0.0f;
}

bool FeintAttackState::Update(Enemy* enemy, float dt)
{
    const Transform* target = enemy->GetTargetTransform();
    Transform e = enemy->GetTransform();

    switch (phase_)
    {
    case Phase::WindUp:
    {
        // 回転薙ぎ払いと同じ構え（Sword）で溜める。
        // プレイヤーは「回転が来る」と思い込む ＝ 読み合いの仕掛け
        if (target) {
            Vector3 toTarget = target->translate - e.translate;
            toTarget.y = 0.0f;
            if (Length(toTarget) > 1e-6f) {
                Vector3 dir = Normalize(toTarget);
                float desiredYaw = std::atan2(dir.x, dir.z);
                e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, param_.turnLerp);
            }
        }

        if (!enemy->IsHitReact()) {
            enemy->SetAnimationIfChanged(enemy->GetAnimSet().Sword);
        }

        timer_ += dt;
        if (timer_ >= param_.windUpTime) {
            timer_ = 0.0f;
            phase_ = Phase::FeintHold;
        }
        break;
    }

    case Phase::FeintHold:
    {
        // 構えたまま完全静止。攻撃判定は出さない。
        // ここでプレイヤーが回避すると、硬直が解ける前に Lunge が刺さる
        timer_ += dt;
        if (timer_ >= param_.feintHoldTime) {
            timer_ = 0.0f;

            // 突き込みの始点・方向を確定（この瞬間のプレイヤー方向へ）
            lungeStart_ = e.translate;
            lungeDir_ = Vector3{ 0.0f, 0.0f, 1.0f };
            if (target) {
                Vector3 toTarget = target->translate - e.translate;
                toTarget.y = 0.0f;
                if (Length(toTarget) > 1e-6f) {
                    lungeDir_ = Normalize(toTarget);
                    e.rotate.y = std::atan2(lungeDir_.x, lungeDir_.z);
                }
            }

            // 本攻撃の判定を展開
            enemy->ActivateAreaAttack(param_.hitRadius);
            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Punch);
            }
            phase_ = Phase::Lunge;
        }
        break;
    }

    case Phase::Lunge:
    {
        // 前方へ素早く突き込む（判定はボスに追従）
        timer_ += dt;
        float t = timer_ / param_.lungeTime;
        if (t > 1.0f) t = 1.0f;

        e.translate = lungeStart_ + lungeDir_ * (param_.lungeDistance * t);

        if (timer_ >= param_.lungeTime) {
            timer_ = 0.0f;
            enemy->DeactivateAreaAttack();
            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
            }
            phase_ = Phase::Recover;
        }
        break;
    }

    case Phase::Recover:
    {
        // 硬直（プレイヤーの反撃チャンス）
        if (!enemy->IsHitReact()) {
            enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
        }

        timer_ += dt;
        if (timer_ >= param_.recoverTime) {
            enemy->SetTransform(e);
            return false; // 完了
        }
        break;
    }
    }

    enemy->SetTransform(e);
    return true; // まだ実行中
}

void FeintAttackState::Exit(Enemy* enemy)
{
    // 中断された場合の保険：判定を必ず閉じる
    enemy->DeactivateAreaAttack();
    if (!enemy->IsHitReact()) {
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
    }
}

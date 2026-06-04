#include "JumpSlamState.h"
#include "Enemy/Enemy.h"
#include "Camera/CameraEffectController.h"
#include <cmath>

JumpSlamState::JumpSlamState(const JumpSlamParam& param)
    : param_(param)
{
}

void JumpSlamState::Enter(Enemy* enemy)
{
    phase_ = Phase::WindUp;
    timer_ = 0.0f;
    baseY_ = enemy->GetTransform().translate.y;
}

bool JumpSlamState::Update(Enemy* enemy, float dt)
{
    const Transform* target = enemy->GetTargetTransform();
    Transform e = enemy->GetTransform();

    switch (phase_)
    {
    case Phase::WindUp:
    {
        // しゃがみ込んでプレイヤーへ向き直る（予備動作＝跳躍の予告）
        if (target) {
            Vector3 to = target->translate - e.translate;
            to.y = 0.0f;
            if (Length(to) > 1e-6f) {
                Vector3 dir = Normalize(to);
                e.rotate.y = std::atan2(dir.x, dir.z);
            }
        }

        if (!enemy->IsHitReact()) {
            enemy->SetAnimationIfChanged(enemy->GetAnimSet().Duck);
        }

        timer_ += dt;
        if (timer_ >= param_.windUpTime) {
            timer_ = 0.0f;

            // 跳躍の始点と着地点をここで確定する
            leapStart_ = e.translate;
            leapStart_.y = baseY_;

            Vector3 dst = leapStart_;
            if (target) {
                Vector3 to = target->translate - leapStart_;
                to.y = 0.0f;
                float d = Length(to);
                // 最大跳躍距離でクランプ（遠すぎる相手には届かない）
                if (d > param_.maxLeapDist) {
                    to = Normalize(to) * param_.maxLeapDist;
                }
                dst = leapStart_ + to;
            }
            dst.y = baseY_;
            leapTarget_ = dst;

            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Jump);
            }
            phase_ = Phase::Leap;
        }
        break;
    }

    case Phase::Leap:
    {
        timer_ += dt;
        float t = timer_ / param_.leapTime;
        if (t > 1.0f) t = 1.0f;

        // XZ は線形補間、Y は放物線（頂点 = jumpHeight）
        e.translate.x = leapStart_.x + (leapTarget_.x - leapStart_.x) * t;
        e.translate.z = leapStart_.z + (leapTarget_.z - leapStart_.z) * t;
        e.translate.y = baseY_ + param_.jumpHeight * 4.0f * t * (1.0f - t);

        // 進行方向を向く
        Vector3 dir = leapTarget_ - leapStart_;
        dir.y = 0.0f;
        if (Length(dir) > 1e-6f) {
            dir = Normalize(dir);
            e.rotate.y = std::atan2(dir.x, dir.z);
        }

        if (timer_ >= param_.leapTime) {
            timer_ = 0.0f;
            e.translate = leapTarget_; // 確実に着地点へ

            // 着地：範囲攻撃判定を展開
            enemy->ActivateAreaAttack(param_.impactRadius);

            // 着地：重い衝撃をカメラ振動で表現する
            if (auto* fx = enemy->GetCameraEffect()) {
                CameraEffectController::ShakeParams sp;
                sp.Duration(param_.shakeDuration)
                  .AmpPos(param_.shakeAmplitude)
                  .Frequency(param_.shakeFrequency)
                  .Damping(2.0f)
                  .Mode(CameraEffectController::ShakeMode::Vertical);
                fx->StartShake(sp);
            }

            if (!enemy->IsHitReact()) {
                enemy->SetAnimationIfChanged(enemy->GetAnimSet().Jump_Land);
            }
            phase_ = Phase::Impact;
        }
        break;
    }

    case Phase::Impact:
    {
        // 着地衝撃の判定が出ている時間。接地を維持する
        e.translate.y = baseY_;

        timer_ += dt;
        if (timer_ >= param_.impactTime) {
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
        // 硬直（プレイヤーの反撃チャンス）。接地を維持する
        e.translate.y = baseY_;
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

void JumpSlamState::Exit(Enemy* enemy)
{
    // 中断された場合の保険：判定を閉じ、宙に浮いたままにしない
    enemy->DeactivateAreaAttack();
    Transform e = enemy->GetTransform();
    e.translate.y = baseY_;
    enemy->SetTransform(e);
    if (!enemy->IsHitReact()) {
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
    }
}

#include "SpinAttackState.h"
#include "Enemy/Enemy.h"
#include "Camera/CameraEffectController.h"
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

SpinAttackState::SpinAttackState(const SpinAttackParam& param)
    : param_(param)
{
}

void SpinAttackState::Enter(Enemy* enemy)
{
    phase_ = Phase::WindUp;
    timer_ = 0.0f;
    // 開始時の接地Yを記憶（ホップはここを基準にする）
    baseY_ = enemy->GetTransform().translate.y;
}

bool SpinAttackState::Update(Enemy* enemy, float dt)
{
    const Transform* target = enemy->GetTargetTransform();
    Transform e = enemy->GetTransform();

    switch (phase_)
    {
    case Phase::WindUp:
    {
        // プレイヤーへ向き直りながら武器を構える（予備動作＝離れろの予告）
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
            // 攻撃判定（範囲）を展開して回転開始
            enemy->ActivateAreaAttack(param_.hitRadius);
            phase_ = Phase::Spin;
        }
        break;
    }

    case Phase::Spin:
    {
        // その場で高速回転。周囲の攻撃判定がプレイヤーを罰する
        e.rotate.y += param_.spinSpeed * dt;

        timer_ += dt;

        // Y方向のホップ：開始でフワッと浮き、終了でドスンと着地する
        float lift = param_.liftHeight;
        if (param_.liftRiseTime > 0.0f && timer_ < param_.liftRiseTime) {
            // 浮き上がり（イーズアウト）
            float t = timer_ / param_.liftRiseTime;       // 0→1
            float eased = 1.0f - (1.0f - t) * (1.0f - t);
            lift = param_.liftHeight * eased;
        }
        else if (param_.liftFallTime > 0.0f &&
                 timer_ > param_.spinTime - param_.liftFallTime) {
            // 着地（イーズイン＝加速して重く落ちる）
            float t = (timer_ - (param_.spinTime - param_.liftFallTime)) / param_.liftFallTime;
            if (t > 1.0f) t = 1.0f;
            lift = param_.liftHeight * (1.0f - t * t);
        }
        e.translate.y = baseY_ + lift;

        if (timer_ >= param_.spinTime) {
            timer_ = 0.0f;
            e.translate.y = baseY_; // 確実に接地させる

            // 着地の瞬間：足元に土煙を発生させる
            enemy->SpawnSpinLandEffect(e.translate);

            // 着地の瞬間：重い衝撃をカメラ振動で表現する
            if (auto* fx = enemy->GetCameraEffect()) {
                CameraEffectController::ShakeParams sp;
                sp.Duration(param_.shakeDuration)
                  .AmpPos(param_.shakeAmplitude)
                  .Frequency(param_.shakeFrequency)
                  .Damping(2.0f)  // 初め強く後半急減衰＝パンチのある揺れ
                  .Mode(CameraEffectController::ShakeMode::Vertical);
                fx->StartShake(sp);
            }

            // 判定終了 → 硬直へ
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

void SpinAttackState::Exit(Enemy* enemy)
{
    // 中断された場合の保険：判定を必ず閉じ、宙に浮いたままにしない
    enemy->DeactivateAreaAttack();
    Transform e = enemy->GetTransform();
    e.translate.y = baseY_;
    enemy->SetTransform(e);
    if (!enemy->IsHitReact()) {
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
    }
}

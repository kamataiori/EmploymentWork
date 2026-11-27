#include "TimeManager.h"
#include <algorithm> // std::max, std::clamp

namespace
{
    // 復帰を滑らかにするためのイージング（Tween::Easing::EaseOutCubic 相当）。
    // 「最初に一気に加速 → 最後はゆっくり通常速度へ」という戻り方になり、
    // カクッと等速で戻すよりも止め明けが自然に感じられる。
    // TimeManager をエンジン基盤の最小依存に保つため、ここに小さく持つ。
    float EaseOutCubic(float t)
    {
        const float u = t - 1.0f;
        return u * u * u + 1.0f;
    }
}

TimeManager* TimeManager::GetInstance()
{
    static TimeManager instance;
    return &instance;
}

void TimeManager::Update(float rawDeltaSeconds)
{
    // 負の値が来ることは想定していないが、一応 0 でクランプ
    rawDeltaSeconds = std::max(rawDeltaSeconds, 0.0f);

    unscaledDeltaTime_ = rawDeltaSeconds;
    unscaledTime_ += unscaledDeltaTime_;

    // ヒットストップは実時間(unscaled)で進め、結果を「時間倍率の上掛け」として合成する。
    // こうすることで、ポーズ(0.0)やボス撃破スロー(0.1)など別用途で設定された
    // baseTimeScale_ を壊さずに、その上から一瞬の“止め”を重ねられる。
    const float hitStopScale = UpdateHitStop(unscaledDeltaTime_);
    const float effectiveScale = baseTimeScale_ * hitStopScale;

    // deltaTime は「基準倍率 × ヒットストップ」を適用
    deltaTime_ = unscaledDeltaTime_ * effectiveScale;
    time_ += deltaTime_;
}

void TimeManager::SetTimeScale(float scale)
{
    // マイナスはおかしいので 0 以上にクランプ
    if (scale < 0.0f) {
        scale = 0.0f;
    }
    baseTimeScale_ = scale;
}

void TimeManager::RequestHitStop(const HitStopRequest& request)
{
    // 不正値ガード（負の時間・範囲外の倍率を弾く）
    HitStopRequest req = request;
    req.minScale = std::clamp(req.minScale, 0.0f, 1.0f);
    req.freezeSeconds = std::max(req.freezeSeconds, 0.0f);
    req.recoverSeconds = std::max(req.recoverSeconds, 0.0f);

    // 連続ヒット時の優先度：
    // 既に進行中のヒットストップより「短い」要求で上書きすると、
    // 演出を途中で打ち切ってしまい手応えが安っぽくなる。
    // そこで「これから止まる合計時間が、現在の残り時間以上になる」要求だけ採用する。
    if (hitStopState_ != HitStopState::Inactive) {
        const float incomingTotal = req.freezeSeconds + req.recoverSeconds;
        if (incomingTotal < HitStopRemainingSeconds()) {
            return; // 弱い要求は無視して既存演出を優先
        }
    }

    hitStopRequest_ = req;
    hitStopElapsed_ = 0.0f;
    // freeze 時間が無いなら、いきなり復帰フェーズから始める
    hitStopState_ = (req.freezeSeconds > 0.0f)
        ? HitStopState::Freezing
        : HitStopState::Recovering;
}

float TimeManager::HitStopRemainingSeconds() const
{
    switch (hitStopState_) {
    case HitStopState::Freezing:
        // 残りの freeze ＋ これから行う recover の全部
        return (hitStopRequest_.freezeSeconds - hitStopElapsed_) + hitStopRequest_.recoverSeconds;
    case HitStopState::Recovering:
        return hitStopRequest_.recoverSeconds - hitStopElapsed_;
    case HitStopState::Inactive:
    default:
        return 0.0f;
    }
}

float TimeManager::UpdateHitStop(float unscaledDeltaSeconds)
{
    switch (hitStopState_) {
    case HitStopState::Inactive:
        return 1.0f;

    case HitStopState::Freezing:
    {
        hitStopElapsed_ += unscaledDeltaSeconds;
        if (hitStopElapsed_ >= hitStopRequest_.freezeSeconds) {
            // フリーズ終了 → 復帰フェーズへ（経過はリセットして数え直す）
            hitStopElapsed_ = 0.0f;
            if (hitStopRequest_.recoverSeconds <= 0.0f) {
                // 復帰時間が無ければそのまま通常速度へ
                hitStopState_ = HitStopState::Inactive;
                return 1.0f;
            }
            hitStopState_ = HitStopState::Recovering;
        }
        // 止めている間は最小倍率を維持
        return hitStopRequest_.minScale;
    }

    case HitStopState::Recovering:
    {
        hitStopElapsed_ += unscaledDeltaSeconds;
        const float duration = hitStopRequest_.recoverSeconds;
        const float t = (duration > 0.0f) ? (hitStopElapsed_ / duration) : 1.0f;
        if (t >= 1.0f) {
            hitStopState_ = HitStopState::Inactive;
            return 1.0f;
        }
        // minScale → 1.0 へイージングで滑らかに加速して戻す
        const float eased = EaseOutCubic(t);
        return hitStopRequest_.minScale + (1.0f - hitStopRequest_.minScale) * eased;
    }
    }

    return 1.0f;
}

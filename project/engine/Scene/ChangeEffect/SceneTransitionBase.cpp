#include "SceneTransitionBase.h"
#include "engine/TimeManager.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

SceneTransitionBase::SceneTransitionBase()
{
    timeManager_ = TimeManager::GetInstance();
    SetActive(false);
}

void SceneTransitionBase::Start(float fadeOutSec, float fadeInSec)
{
    fadeOutSec_ = std::max(0.0f, fadeOutSec);
    fadeInSec_ = std::max(0.0f, fadeInSec);

    timer_ = 0.0f;
    phase_ = Phase::FadeOut;
    hasSwitched_ = false;

    SetActive(true);
}

void SceneTransitionBase::SetOnSwitch(std::function<void()> callback)
{
    onSwitch_ = std::move(callback);
}

void SceneTransitionBase::SetOnFinish(std::function<void()> callback)
{
    onFinish_ = std::move(callback);
}

bool SceneTransitionBase::IsRunning() const
{
    return phase_ != Phase::None && phase_ != Phase::Done;
}

void SceneTransitionBase::Update()
{
    if (!IsActive()) {
        return;
    }

    const float dt = GetDeltaTime();

    switch (phase_) {
    case Phase::FadeOut:
    {
        timer_ += dt;
        const float p = Normalize(timer_, fadeOutSec_);
        OnFadeOut(p);

        if (timer_ >= fadeOutSec_) {
            timer_ = 0.0f;

            // HoldがあるならHoldへ
            if (holdSec_ > 0.0f) {
                phase_ = Phase::Hold;
            }
            else {
                phase_ = Phase::Switch;
            }
        }
        break;
    }

    case Phase::Hold:
    {
        timer_ += dt;

        // 暗転状態は保持したいので、見た目は完全暗転側に固定
        // ここは派生の実装が「FadeOut=1.0」が暗転完了状態になる想定
        OnFadeOut(1.0f);

        if (timer_ >= holdSec_) {
            timer_ = 0.0f;
            phase_ = Phase::Switch;
        }
        break;
    }

    case Phase::Switch:
    {
        // ここは一回だけ
        if (!hasSwitched_) {
            hasSwitched_ = true;
            if (onSwitch_) {
                onSwitch_();
            }
        }

        timer_ = 0.0f;
        phase_ = Phase::FadeIn;
        break;
    }

    case Phase::FadeIn:
    {
        timer_ += dt;
        const float p = Normalize(timer_, fadeInSec_);
        OnFadeIn(p);

        if (timer_ >= fadeInSec_) {
            phase_ = Phase::Done;
            SetActive(false);

            if (onFinish_) {
                onFinish_();
            }
        }
        break;
    }

    default:
        break;
    }
}

void SceneTransitionBase::SetHoldSeconds(float sec)
{
    holdSec_ = std::max(0.0f, sec);
}

float SceneTransitionBase::GetDeltaTime() const
{
    // 遷移演出は時間停止の影響を受けない方が扱いやすい
    return timeManager_->GetUnscaledDeltaTime();
}

float SceneTransitionBase::Normalize(float t, float duration)
{
    if (duration <= 0.0f) {
        return 1.0f;
    }
    return std::clamp(t / duration, 0.0f, 1.0f);
}

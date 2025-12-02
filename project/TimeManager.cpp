#include "TimeManager.h"
#include <algorithm> // std::max

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

    // deltaTime は timeScale を適用
    deltaTime_ = unscaledDeltaTime_ * timeScale_;
    time_ += deltaTime_;
}

void TimeManager::SetTimeScale(float scale)
{
    // マイナスはおかしいので 0 以上にクランプ
    if (scale < 0.0f) {
        scale = 0.0f;
    }
    timeScale_ = scale;
}

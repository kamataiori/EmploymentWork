#include "SceneTransitionService.h"
#include "engine/UI/UIManager.h"
#include "SceneTransitionStates.h"
#include <cassert>

SceneTransitionService::SceneTransitionService()
{
    factory_[TransitionType::None] = [] { return std::make_unique<NoneTransitionState>(); };
    factory_[TransitionType::Fade] = [] { return std::make_unique<FadeTransitionState>(); };
    factory_[TransitionType::Shutter] = [] { return std::make_unique<ShutterTransitionState>(); };
}

SceneTransitionService::~SceneTransitionService() = default;

void SceneTransitionService::StartTransition(
    const std::string& nextSceneName,
    const TransitionRequest& req,
    std::function<void(const std::string&)> onSwitch,
    std::function<void()> onFinish
)
{
    assert(uiManager_);

    // 二重開始を防ぐ
    if (isTransitioning_) {
        return;
    }

    isTransitioning_ = true;

    // Switchは一回だけ呼ぶ想定なので、service側で閉じ込める
    auto onSwitchOnce = [onSwitch, nextSceneName]() {
        if (onSwitch) {
            onSwitch(nextSceneName);
        }
        };

    auto onFinishOnce = [this, onFinish]() {
        // 先にservice内部のフラグを戻す
        isTransitioning_ = false;

        if (onFinish) {
            onFinish();
        }
        };

    //==================================================
    // type -> state をレジストリから生成
    //==================================================
    auto it = factory_.find(req.type);
    if (it == factory_.end()) {
        // 未登録なら None 扱い（安全）
        it = factory_.find(TransitionType::None);
    }

    state_ = it->second();
    state_->Enter(uiManager_, req, std::move(onSwitchOnce), std::move(onFinishOnce));
}

#include "SceneTransitionService.h"
#include "engine/UI/UIManager.h"
#include "engine/Scene/ChangeEffect/Fade/FadeTransition.h"
#include <engine/Scene/ChangeEffect/Shutter/ShutterTransition.h>
#include <cassert>

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

    switch (req.type) {
    case TransitionType::Fade:
        CreateAndEnqueueFade(req, onSwitchOnce, onFinishOnce);
        break;

    case TransitionType::Shutter:
        CreateAndEnqueueShutter(req, onSwitchOnce, onFinishOnce);
        break;


    default:
        // 演出なしなら即切替して即終了
        onSwitchOnce();
        onFinishOnce();
        break;
    }
}

void SceneTransitionService::CreateAndEnqueueFade(
    const TransitionRequest& req,
    std::function<void()> onSwitchOnce,
    std::function<void()> onFinishOnce
)
{
    auto fade = std::make_unique<FadeTransition>();
    fade->Initialize("Resources/Black.png", { 1280.0f, 720.0f }, 100000);

    fade->SetOnSwitch(std::move(onSwitchOnce));
    fade->SetOnFinish(std::move(onFinishOnce));

    fade->Start(req.fadeOutSec, req.fadeInSec);

    uiManager_->Add(std::move(fade));
}

void SceneTransitionService::CreateAndEnqueueShutter(const TransitionRequest& req, std::function<void()> onSwitchOnce, std::function<void()> onFinishOnce)
{
    auto shutter = std::make_unique<ShutterTransition>();

    shutter->Initialize(
        "Resources/Black.png",
        { 1280.0f, 720.0f },
        100000
    );

    shutter->SetHoldSeconds(0.1f);

    shutter->SetOnSwitch(std::move(onSwitchOnce));
    shutter->SetOnFinish(std::move(onFinishOnce));

    shutter->Start(req.fadeOutSec, req.fadeInSec);

    uiManager_->Add(std::move(shutter));
}

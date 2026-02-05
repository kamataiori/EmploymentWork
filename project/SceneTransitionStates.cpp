#include "SceneTransitionStates.h"
#include "engine/UI/UIManager.h"
#include "engine/Scene/ChangeEffect/Fade/FadeTransition.h"
#include <engine/Scene/ChangeEffect/Shutter/ShutterTransition.h>

namespace {
    // ここは今の実装を踏襲
    constexpr const char* kBlackTex = "Resources/Black.png";
    constexpr float kScreenW = 1280.0f;
    constexpr float kScreenH = 720.0f;
    constexpr int   kZOrder = 100000;
}

void FadeTransitionState::Enter(
    UIManager* ui,
    const TransitionRequest& req,
    std::function<void()> onSwitchOnce,
    std::function<void()> onFinishOnce
)
{
    auto fade = std::make_unique<FadeTransition>();
    fade->Initialize(kBlackTex, { kScreenW, kScreenH }, kZOrder);

    fade->SetOnSwitch(std::move(onSwitchOnce));
    fade->SetOnFinish(std::move(onFinishOnce));

    fade->Start(req.fadeOutSec, req.fadeInSec);

    ui->Add(std::move(fade));
}

void ShutterTransitionState::Enter(
    UIManager* ui,
    const TransitionRequest& req,
    std::function<void()> onSwitchOnce,
    std::function<void()> onFinishOnce
)
{
    auto shutter = std::make_unique<ShutterTransition>();
    shutter->Initialize(kBlackTex, { kScreenW, kScreenH }, kZOrder);

    shutter->SetHoldSeconds(0.1f);

    shutter->SetOnSwitch(std::move(onSwitchOnce));
    shutter->SetOnFinish(std::move(onFinishOnce));

    shutter->Start(req.fadeOutSec, req.fadeInSec);

    ui->Add(std::move(shutter));
}

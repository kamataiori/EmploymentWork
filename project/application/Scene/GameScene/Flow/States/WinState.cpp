#include "Flow/States/WinState.h"
#include "Flow/GameFlowContext.h"

#include "Player.h"
#include "FollowCamera.h"
#include "Camera/CameraEffectController.h"
#include "engine/TimeManager.h"
#include <numbers>

void WinState::Enter(GameFlowContext& ctx)
{
    // FollowCamera の通常追従を切り、カメラをこの演出に明け渡す
    ctx.followCameraLocked = true;

    CameraEffectController::OrbitParams orbit{};
    orbit
        .Center(ctx.player->GetTransform().translate)
        .Angle(std::numbers::pi_v<float> / kOrbitAngleRatio)
        .Duration(kOrbitDuration)
        .Easing(Tween::Easing::EaseInExpo);

    ctx.cameraEffect->StartOrbitMove(ctx.followCamera, orbit);

    TimeManager::GetInstance()->SetTimeScale(kSlowMotionScale);

    zoomDelayTimer_ = kZoomDelay;
    zoomTimer_ = 0.0f;
    zoomStarted_ = false;
}

void WinState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // スロー中なので、演出の進みもスロー時間で測る（見た目の尺と一致させる）
    const float dt = TimeManager::GetInstance()->GetDeltaTime();

    if (!zoomStarted_) {
        zoomDelayTimer_ -= dt;
        if (zoomDelayTimer_ <= 0.0f) {
            CameraEffectController::ZoomParams zoom{};
            zoom
                .UseCurrentFov(true)
                .ToFov(std::numbers::pi_v<float> / kZoomFovRatio)
                .Duration(kZoomDuration)
                .Easing(Tween::Easing::EaseOutExpo);

            ctx.cameraEffect->StartZoom(zoom);
            zoomStarted_ = true;
            zoomTimer_ = kZoomDuration;
        }
        return;
    }

    if (zoomTimer_ > 0.0f) {
        zoomTimer_ -= dt;
        if (zoomTimer_ <= 0.0f) {
            // 演出が終わったので時間を等倍へ戻す
            TimeManager::GetInstance()->SetTimeScale(1.0f);
        }
    }
}

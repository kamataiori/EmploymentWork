#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"

#include "SceneManager.h"
#include "PostEffectManager.h"
#include "engine/TimeManager.h"

void LoseState::Enter(GameFlowContext& ctx)
{
    (void)ctx;

    vignetteTimer_ = 0.0f;

    PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
    PostEffectManager::GetInstance()->VignetteInitialize(
        kVignetteScale, 0.0f, { 0.0f, 0.0f, 0.0f }
    );
}

void LoseState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 暗転はゲーム時間のスケールに影響されたくないので実時間で進める
    vignetteTimer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();

    float t = vignetteTimer_ / kVignetteDuration;
    if (t > 1.0f) t = 1.0f;

    PostEffectManager::GetInstance()->SetVignettePower(kVignettePower * t);

    if (t >= 1.0f) {
        // 次のシーンにビネットを持ち越さないよう戻してから遷移する
        PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
        SceneManager::GetInstance()->ChangeScene("TITLE");
    }
}

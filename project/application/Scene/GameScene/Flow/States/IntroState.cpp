#include "Flow/States/IntroState.h"
#include "Flow/States/SwarmWaveState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Enemy.h"
#include "SceneController.h"
#include "SkyBox.h"
#include "Object3d.h"
#include "engine/TimeManager.h"

namespace {
    // カウントダウン表示の見た目
    constexpr float kSpriteWidth  = 320.0f;
    constexpr float kSpriteHeight = 180.0f;
    const Vector2   kSpritePos    = { 1280.0f * 0.5f, 720.0f * 0.25f };
    const Vector4   kSpriteColor  = { 1.0f, 0.0f, 0.0f, 1.0f };

    void SetupCueSprite(Sprite& sprite, const std::string& texPath)
    {
        sprite.Initialize(texPath);
        sprite.SetSize({ kSpriteWidth, kSpriteHeight });
        sprite.SetAnchorPoint({ 0.5f, 0.5f });
        sprite.SetPosition(kSpritePos);
        sprite.SetColor(kSpriteColor);
    }
}

IntroState::IntroState()
{
    for (int i = 0; i <= kCountMax; ++i) {
        countSprites_[i] = std::make_unique<Sprite>();
        SetupCueSprite(*countSprites_[i], "Resources/count_" + std::to_string(i) + ".png");
    }

    startSprite_ = std::make_unique<Sprite>();
    SetupCueSprite(*startSprite_, "Resources/start.png");
}

void IntroState::Enter(GameFlowContext& ctx)
{
    ctx.player->SetInputLocked(true);
}

void IntroState::Exit(GameFlowContext& ctx)
{
    ctx.player->SetInputLocked(false);
}

void IntroState::Update(GameFlowContext& ctx)
{
    // 演出はゲーム時間のスケールに左右されたくないので実時間で進める
    const float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

    // 見た目だけ動かす。敵はAIも弾も止めたまま（UpdateVisual）
    ctx.stage->Update();
    ctx.skybox->Update();
    ctx.ground->Update();
    ctx.sky->Update();
    ctx.player->Update();
    ctx.boss->UpdateVisual();

    if (!showingStartCue_) {
        countdownTimer_ += dt;
        if (countdownTimer_ >= kCountPerSec) {
            countdownTimer_ -= kCountPerSec;
            --countdownNum_;

            if (countdownNum_ < 0) {
                showingStartCue_ = true;
                startDisplayTimer_ = 0.0f;
            }
        }
        return;
    }

    startDisplayTimer_ += dt;
    if (startDisplayTimer_ >= kStartDisplaySec) {
        // 第1波（群れ突進の雑魚）から開始する
        ctx.flow->ChangeState(std::make_unique<SwarmWaveState>());
    }
}

void IntroState::UIDraw(GameFlowContext& ctx)
{
    (void)ctx;

    // 演出中はゲームのUI（HP・スキル）を出さず、カウントダウンだけを見せる
    if (showingStartCue_) {
        startSprite_->Update();
        startSprite_->Draw();
        return;
    }

    if (countdownNum_ >= 0 && countdownNum_ <= kCountMax) {
        countSprites_[countdownNum_]->Update();
        countSprites_[countdownNum_]->Draw();
    }
}

#include "TutorialStateMove.h"
#include "application/Scene/TutorialScene/TutorialController.h"
#include "application/Scene/TutorialScene/TutorialStateJump.h"

TutorialStateMove::TutorialStateMove(TutorialController* ctx,
    std::string guidePath,
    float drainSec)
    : BaseTutorialState("Move", ctx),
    guidePath_(std::move(guidePath)),
    drainSec_(drainSec) {
}

void TutorialStateMove::Enter()
{
    ratio_ = 1.0f;

    // WASD説明画像を表示
    ctx_->EmitGuide(guidePath_);

    // ゲージ初期化
    ctx_->EmitMoveGauge(ratio_);
}

void TutorialStateMove::Exit()
{
    // ゲージ非表示（Scene側で0なら隠す想定）
    ctx_->EmitMoveGauge(0.0f);
}

void TutorialStateMove::Update(float dt)
{
    // WASD を押している間だけゲージを減らす
    if (Call(ctx_->Act().isMoving)) {
        ratio_ -= dt / drainSec_;
        if (ratio_ < 0.0f) ratio_ = 0.0f;
        ctx_->EmitMoveGauge(ratio_);
    }

    // ゲージが空になったらジャンプ状態へ
    if (ratio_ <= 0.0f) {
        ctx_->ChangeState(
            std::make_unique<TutorialStateJump>(ctx_,
                "Resources/space_jump.png",
                3)
        );
    }
}

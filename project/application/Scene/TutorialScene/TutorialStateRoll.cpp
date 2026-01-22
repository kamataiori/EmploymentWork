#include "TutorialStateRoll.h"
#include "application/Scene/TutorialScene/TutorialController.h"
// 次の状態
#include "application/Scene/TutorialScene/TutorialStateDash.h"

TutorialStateRoll::TutorialStateRoll(
    TutorialController* ctx,
    std::string guidePath,
    int need
)
    : BaseTutorialState("Roll", ctx),
    guidePath_(std::move(guidePath)),
    need_(need)
{
}

void TutorialStateRoll::Enter()
{
       // 状態に入った瞬間の処理
       // カウントをリセット
       // ローリング説明用のガイド画像を表示
    count_ = 0;
    ctx_->EmitGuide(guidePath_);
}

void TutorialStateRoll::Update(float /*dt*/)
{
       // ローリング入力（押した瞬間）を検知
       // 指定回数に達したら次の状態へ

    if (Call(ctx_->Act().rollTriggered)) {
        ++count_;
    }

    if (count_ >= need_) {
        // 次：ダッシュチュートリアルへ
        ctx_->ChangeState(
            std::make_unique<TutorialStateDash>(
                ctx_,
                "Resources/rmb_dash.png",
                3
            )
        );
    }
}

#include "TutorialStateDash.h"
#include "application/Scene/TutorialScene/TutorialController.h"
// 次の状態
#include "TutorialStateFinal.h"

TutorialStateDash::TutorialStateDash(
	TutorialController* ctx,
	std::string guidePath,
	int need
)
	: BaseTutorialState("Dash", ctx),
	guidePath_(std::move(guidePath)),
	need_(need)
{
}

void TutorialStateDash::Enter()
{
	// 状態に入った瞬間の処理
	// カウントをリセット
	// ダッシュ説明用ガイド画像を表示
	count_ = 0;
	ctx_->EmitGuide(guidePath_);
}

void TutorialStateDash::Update(float /*dt*/)
{
	// ダッシュ入力（押した瞬間）を検知
	// 指定回数に達したら次の状態へ

	if (Call(ctx_->Act().dashTriggered)) {
		++count_;
	}

	if (count_ >= need_) {
		// 次：最後のメッセージ（敵を倒そう！）を表示して終了へ
		ctx_->ChangeState(
			std::make_unique<TutorialStateFinal>(
				ctx_,
				"Resources/defeat_enemy.png",
				1.5f
			)
		);
	}
}

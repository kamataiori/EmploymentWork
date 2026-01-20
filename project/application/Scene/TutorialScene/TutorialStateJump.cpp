#include "application/Scene/TutorialScene/TutorialStateJump.h"
#include "application/Scene/TutorialScene/TutorialController.h"
// 次の状態
#include "application/Scene/TutorialScene/TutorialStateAttack.h"

TutorialStateJump::TutorialStateJump(
	TutorialController* ctx,
	std::string guidePath,
	int need
)
	: BaseTutorialState("Jump", ctx),
	guidePath_(std::move(guidePath)),
	need_(need)
{
}

void TutorialStateJump::Enter()
{
	// 状態に入った瞬間の処理
	// カウントをリセット
	// ジャンプ説明用のガイド画像を表示
	count_ = 0;
	ctx_->EmitGuide(guidePath_);
}

void TutorialStateJump::Update(float /*dt*/)
{
	// ジャンプ入力（押した瞬間）を検知
	// 指定回数に達したら次の状態へ

	if (Call(ctx_->Act().jumpTriggered)) {
		++count_;
	}

	if (count_ >= need_) {
		// 次：攻撃チュートリアルへ
		ctx_->ChangeState(
			std::make_unique<TutorialStateAttack>(
				ctx_,
				"Resources/lmb_attack.png",
				3
			)
		);
	}
}

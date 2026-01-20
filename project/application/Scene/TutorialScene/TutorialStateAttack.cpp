#include "application/Scene/TutorialScene/TutorialStateAttack.h"
#include <utility>
#include "application/Scene/TutorialScene/TutorialController.h"
// 次の状態
#include "application/Scene/TutorialScene/TutorialStateRoll.h"

TutorialStateAttack::TutorialStateAttack(
	TutorialController* ctx,
	std::string guidePath,
	int need
)
	: BaseTutorialState("Attack", ctx),
	guidePath_(std::move(guidePath)),
	need_(need)
{
}

void TutorialStateAttack::Enter()
{
	// 状態に入った瞬間の処理
	// カウントをリセット
	// 攻撃説明用のガイド画像を表示
	count_ = 0;
	ctx_->EmitGuide(guidePath_);
}

void TutorialStateAttack::Update(float /*dt*/)
{
	// 攻撃入力（押した瞬間）を検知
	// 指定回数に達したら次の状態へ

	if (Call(ctx_->Act().attackTriggered)) {
		++count_;
	}

	if (count_ >= need_) {
		// 次：ローリングチュートリアルへ
		ctx_->ChangeState(
			std::make_unique<TutorialStateRoll>(
				ctx_,
				"Resources/e_roll.png",
				3
			)
		);
	}
}

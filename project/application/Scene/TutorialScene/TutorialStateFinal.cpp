#include "TutorialStateFinal.h"
#include "application/Scene/TutorialScene/TutorialController.h"

TutorialStateFinal::TutorialStateFinal(
	TutorialController* ctx,
	std::string guidePath,
	float waitSec
)
	: BaseTutorialState("Final", ctx),
	guidePath_(std::move(guidePath)),
	waitSec_(waitSec)
{
}

void TutorialStateFinal::Enter()
{
	// 状態に入った瞬間の処理
	// 経過時間をリセット
	// 最後のメッセージを表示

	acc_ = 0.0f;
	ctx_->EmitGuide(guidePath_);
}

void TutorialStateFinal::Update(float dt)
{
	// 時間を加算
	// 一定時間経過したら Finish を呼ぶ

	acc_ += dt;

	if (acc_ >= waitSec_) {
		ctx_->Finish();
	}
}

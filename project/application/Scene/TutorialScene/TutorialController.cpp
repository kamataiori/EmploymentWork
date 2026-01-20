#include "TutorialController.h"
#include "application/Scene/TutorialScene/BaseTutorialState.h"
// 初期状態（Move）を生成するために include
#include "application/Scene/TutorialScene/TutorialStateMove.h"

TutorialController::TutorialController() = default;
TutorialController::~TutorialController() = default;

void TutorialController::Initialize(const Actions& actions, const Hooks& hooks)
{
	// Scene から受け取った入力・通知用関数を保存
	actions_ = actions;
	hooks_ = hooks;

	finished_ = false;

	// 最初の状態をセットする
	// Enemy.cpp の Initialize で
	// ChangeState(std::make_unique<EnemyStateApproach>(this));を呼んでいるのと同じ考え方
	ChangeState(
		std::make_unique<TutorialStateMove>(
			this,
			"Resources/wasd_mouse.png", // WASD + マウス説明
			2.0f                        // ゲージが0になるまでの必要秒数
		)
	);
}

void TutorialController::Update(float dt)
{
	// 終了済み、または状態が無ければ何もしない
	if (finished_ || !state_) {
		return;
	}

	// 現在の状態に処理を委譲
	state_->Update(dt);
}

const char* TutorialController::GetStateName() const
{
	// デバッグ用：現在の状態名を返す
	return state_ ? state_->Name() : "None";
}

void TutorialController::EmitGuide(const std::string& path)
{

	// ガイド画像の変更通知
	// TutorialController 自身は描画を行わず、Scene 側に通知するだけ
	if (hooks_.onGuideChanged) {
		hooks_.onGuideChanged(path);
	}
}

void TutorialController::EmitMoveGauge(float ratio)
{
	// WASD 移動用ゲージの更新通知
	// ratio : 0.0f（空）〜 1.0f（満タン）
	if (hooks_.onMoveGauge) {
		hooks_.onMoveGauge(ratio);
	}
}

void TutorialController::Finish()
{
	// チュートリアル完了
	// Scene 側で GameScene へ遷移させる想定

	finished_ = true;

	if (hooks_.onFinished) {
		hooks_.onFinished();
	}
}

void TutorialController::ChangeState(std::unique_ptr<BaseTutorialState> next)
{
	// 状態切り替え処理
	// 前の状態があれば Exit
	// 新しい状態をセット
	// Enter を呼ぶ

	if (state_) {
		state_->Exit();
	}

	state_ = std::move(next);

	if (state_) {
		state_->Enter();
	}
}

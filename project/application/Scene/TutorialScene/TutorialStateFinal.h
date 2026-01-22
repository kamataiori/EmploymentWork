#pragma once
#include <application/Scene/TutorialScene/BaseTutorialState.h>
#include <string>

// 最後のメッセージを一定時間表示する状態
// 指定時間 waitSec が経過したら TutorialController::Finish() を呼ぶ
// Scene 側は onFinished を受け取って GameScene へ遷移する想定

class TutorialStateFinal final : public BaseTutorialState {
public:

	// ctx       : 管理元の TutorialController
	// guidePath : 表示するガイド画像のパス
	// waitSec   : 表示して待つ秒数
	TutorialStateFinal(
		TutorialController* ctx,
		std::string guidePath,
		float waitSec
	);

	// 状態に入った瞬間の処理
	void Enter() override;

	// 毎フレーム処理
	void Update(float dt) override;

private:
	std::string guidePath_; // 最後の説明用ガイド画像
	float waitSec_ = 1.5f;  // 待機秒数
	float acc_ = 0.0f;      // 経過時間
};

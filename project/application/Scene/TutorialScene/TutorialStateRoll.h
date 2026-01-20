#pragma once
#include "application/Scene/TutorialScene/BaseTutorialState.h"
#include <string>
#include <functional>

// ローリング操作（Eキー）を教えるチュートリアル状態
// rollTriggered（押した瞬間）を監視
// 指定回数ローリングしたら次の状態へ遷移

class TutorialStateRoll final : public BaseTutorialState {
public:

	// ctx       : 管理元の TutorialController
	// guidePath : 表示するガイド画像のパス
	// need      : ローリングが必要な回数
	TutorialStateRoll(
		TutorialController* ctx,
		std::string guidePath,
		int need
	);

	// 状態に入った瞬間の処理
	void Enter() override;

	// 毎フレーム処理
	void Update(float dt) override;

private:

	// std::function を安全に呼び出すためのヘルパー関数
	static bool Call(const std::function<bool()>& f)
	{
		return f ? f() : false;
	}

	std::string guidePath_; // ローリング説明用ガイド画像
	int need_ = 3;          // 必要なローリング回数
	int count_ = 0;         // 現在のローリング回数
};

#pragma once
#include "application/Scene/TutorialScene/BaseTutorialState.h"
#include <string>
#include <functional>

// 攻撃操作（左クリック）を教えるチュートリアル状態
// attackTriggered（押した瞬間）を監視
//指定回数攻撃したら次の状態へ遷移

class TutorialStateAttack final : public BaseTutorialState {
public:
	// ctx       : 管理元の TutorialController
	// guidePath : 表示するガイド画像のパス
	// need      : 攻撃が必要な回数
	TutorialStateAttack(
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
	// ・関数が未設定（nullptr）の場合は false を返す
	static bool Call(const std::function<bool()>& f)
	{
		return f ? f() : false;
	}

	std::string guidePath_; // 攻撃説明用ガイド画像
	int need_ = 3;          // 必要な攻撃回数
	int count_ = 0;         // 現在の攻撃回数
};

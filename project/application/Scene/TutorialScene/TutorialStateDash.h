#pragma once
#include <string>
#include <functional>
#include <application/Scene/TutorialScene/BaseTutorialState.h>

// ダッシュ操作（右クリック）を教えるチュートリアル状態
// dashTriggered（押した瞬間）を監視
 // 指定回数ダッシュしたら次の状態（Final）へ遷移
class TutorialStateDash final : public BaseTutorialState {
public:

	// ctx       : 管理元の TutorialController
	// guidePath : 表示するガイド画像のパス
	// need      : ダッシュが必要な回数
	TutorialStateDash(
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
	// 未設定なら false を返す
	static bool Call(const std::function<bool()>& f)
	{
		return f ? f() : false;
	}

	std::string guidePath_; // ダッシュ説明用ガイド画像
	int need_ = 3;          // 必要なダッシュ回数
	int count_ = 0;         // 現在のダッシュ回数
};

#pragma once
#include "application/Scene/TutorialScene/BaseTutorialState.h"
#include <functional>

// SPACE ジャンプチュートリアル状態
// ジャンプ入力を N 回検知したら次へ

class TutorialStateJump final : public BaseTutorialState {
public:
	TutorialStateJump(TutorialController* ctx,
		std::string guidePath,
		int need);

	void Enter() override;
	void Update(float dt) override;

private:
	static bool Call(const std::function<bool()>& f) { return f ? f() : false; }

	std::string guidePath_;
	int need_ = 3;
	int count_ = 0;
};

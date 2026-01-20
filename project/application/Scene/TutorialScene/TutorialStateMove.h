#pragma once
#include "BaseTutorialState.h"
#include <functional>

// WASD 移動チュートリアル状態
// WASD を押している間だけゲージが減る
// ゲージが 0 になったら次の状態へ

class TutorialStateMove final : public BaseTutorialState {
public:
	TutorialStateMove(TutorialController* ctx,
		std::string guidePath,
		float drainSec);

	void Enter() override;
	void Exit() override;
	void Update(float dt) override;

private:
	// std::function 安全呼び出し
	static bool Call(const std::function<bool()>& f) { return f ? f() : false; }

	std::string guidePath_; // WASD説明画像
	float drainSec_ = 2.0f; // ゲージを0にするまでの必要秒数
	float ratio_ = 1.0f;    // ゲージ残量（1.0 → 0.0）
};

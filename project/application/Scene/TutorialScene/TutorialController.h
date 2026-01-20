#pragma once
#include <functional>
#include <memory>
#include <string>

class BaseTutorialState;

// チュートリアル全体を管理するクラス
// 現在のチュートリアル状態を1つ保持
// 状態の切り替え
// Sceneへの通知（UI表示など）を担当する

class TutorialController {
public:

	TutorialController();
	~TutorialController();

	// 入力情報
	// Scene 側から std::function で注入される
	struct Actions {
		std::function<bool()> isMoving;
		std::function<bool()> jumpTriggered;
		std::function<bool()> attackTriggered;
		std::function<bool()> rollTriggered;
		std::function<bool()> dashTriggered;
	};

	// Scene への通知用フック
	// TutorialController は「描画しない」
	// Scene 側に通知するだけ

	struct Hooks {
		std::function<void(const std::string&)> onGuideChanged;
		std::function<void(float)> onMoveGauge;
		std::function<void()> onFinished;
	};

	// 初期化（Scene から呼ばれる）
	void Initialize(const Actions& actions, const Hooks& hooks);

	// 毎フレーム更新
	void Update(float dt);

	// 現在の状態名（デバッグ用）
	const char* GetStateName() const;

	// ガイド画像表示通知
	void EmitGuide(const std::string& path);

	// 移動ゲージ更新通知
	void EmitMoveGauge(float ratio);

	// チュートリアル終了通知
	void Finish();

	// 状態切り替え
	void ChangeState(std::unique_ptr<BaseTutorialState> next);

	// 入力参照
	const Actions& Act() const { return actions_; }

private:
	Actions actions_{};
	Hooks hooks_{};

	// 現在のチュートリアル状態
	std::unique_ptr<BaseTutorialState> state_;

	bool finished_ = false;
};

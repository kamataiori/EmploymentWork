#include "Framework.h"
#include "SceneFactory.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <deque>

class MyGame : public Framework
{
public:
	//------メンバ関数------//

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;


	/// <summary>
	/// 
	/// </summary>
	void ApplyImGuiStyle();

	void DrawUnityLayout();
	void DrawCenterPanel();
	void DrawLeftPanels();
	void DrawRightPanels();
	void DrawBottomPanel();

private:

	std::unique_ptr<ImGuiManager> imGuiManager_ = nullptr;

	//std::unique_ptr<OffscreenRendering> offscreenRendering = std::make_unique<OffscreenRendering>();
	//std::unique_ptr<PostEffect> postEffect = std::make_unique<PostEffect>();


	bool useUnityLayout_ = true;
	bool unityDockInitialized_ = false;
	int dockLayoutDelay_ = 0;

public:
	// FPS関係
	float GetFPS() const { return fps_; }
	float GetFrameTimeMs() const { return frameTimeMs_; }
	float GetAverageFPS() const { return averageFps_; }

private:
	// FPS計測用
	std::chrono::steady_clock::time_point lastFrameTime_ = std::chrono::steady_clock::now();
	float fps_ = 0.0f;
	float frameTimeMs_ = 0.0f;

	// 平均FPS用（直近60フレームの履歴）
	std::deque<float> fpsHistory_;
	float averageFps_ = 0.0f;
	static constexpr size_t kFpsHistorySize = 60;

};

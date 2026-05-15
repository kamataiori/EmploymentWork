#include "Framework.h"
#include "SceneFactory.h"
#include "GlobalVariables.h"
#include "engine/UI/UIManager.h"
#include "engine/Scene/ChangeEffect/SceneTransitionService.h"
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"
#include "PlayModeState.h"

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#include "EditorLayout.h"
#endif // USE_IMGUI

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


private:

#ifdef USE_IMGUI

	std::unique_ptr<ImGuiManager> imGuiManager_ = nullptr;

	std::unique_ptr<EditorLayout> editorLayout_ = nullptr;

#endif // USE_IMGUI

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

	// 平均FPS用
	std::deque<float> fpsHistory_;
	float averageFps_ = 0.0f;
	static constexpr size_t kFpsHistorySize = 60;

	float deltaTime_ = 0.0f;  // 毎フレームの経過秒数
	std::chrono::steady_clock::time_point prevTime_;

	// UIはアプリ全体で1つ
	std::unique_ptr<UIManager> uiManager_;

	// 遷移演出の生成・管理（SceneManagerは所有しない）
	std::unique_ptr<SceneTransitionService> transitionService_;

	// 再生/停止 状態 (Play/Stop)
	// EditorLayout と SceneManager が参照する
	std::unique_ptr<PlayModeState> playModeState_;
};

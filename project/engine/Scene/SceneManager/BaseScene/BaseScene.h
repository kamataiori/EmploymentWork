#pragma once
#include "Light.h"
#include <chrono>
#include "PostEffect.h"
#include "SceneController.h"

// 前方宣言
class SceneManager;

class BaseScene
{
public:
	//------メンバ関数------

	BaseScene() {
		AddLeftDockWindow("FPS");  // ← コンストラクタで固定Dock登録
	}

	//仮想デストラクタ
	virtual ~BaseScene() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize() = 0;

	/// <summary>
	/// 終了
	/// </summary>
	virtual void Finalize() = 0;

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// 背景描画
	/// </summary>
	virtual void BackGroundDraw() = 0;

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw() = 0;

	/// <summary>
	/// 前景描画
	/// </summary>
	virtual void ForeGroundDraw() = 0;

	/// <summary>
	/// デバッグ用（Imguiなどはこちらへ）
	/// </summary>
	virtual void Debug() = 0;

	/// <summary>
	/// シーンマネージャーをシーンに貸し出すためのSetter
	/// </summary>
	/// <param name="sceneManager"></param>
	virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

	/// <summary>
	/// Lightのゲッター
	/// </summary>
	Light* GetLight() const {
		return light.get();  // unique_ptr から Light ポインタを返す
	}

	// 描画ループ内で使用
	void ShowFPS() {
		//ImGui::Begin("FPS Display"); // ウィンドウを開始
		//ImGuiIO& io = ImGui::GetIO(); // ImGuiのIOオブジェクトを取得
		//ImGui::Text("FPS: %.1f", io.Framerate); // FPSを表示
		//ImGui::End(); // ウィンドウを終了

		// 現在の時間を取得
		auto now = std::chrono::steady_clock::now();

		// 経過時間を計算
		std::chrono::duration<float> deltaTime = now - lastFrameTime_;
		lastFrameTime_ = now;

		// FPSを計算
		fps_ = 1.0f / deltaTime.count();

		// ImGuiで表示
		ImGui::Begin("FPS Display");
		ImGui::Text("Current FPS: %.2f", fps_);
		ImGui::End();
	}

public:
	// Dock候補登録用（MyGame側のDockBuilderで使われる）
	const std::vector<std::string>& GetLeftDockWindows() const { return leftDockWindows_; }
	const std::vector<std::string>& GetRightDockWindows() const { return rightDockWindows_; }
	const std::vector<std::string>& GetBottomDockWindows() const { return bottomDockWindows_; }

	void AddLeftDockWindow(const std::string& name) { leftDockWindows_.push_back(name); }
	void AddRightDockWindow(const std::string& name) { rightDockWindows_.push_back(name); }
	void AddBottomDockWindow(const std::string& name) { bottomDockWindows_.push_back(name); }

	void SetEnableDockedImGui(bool enable) { enableDockedImGui_ = enable; }
	bool IsDockedImGuiEnabled() const { return enableDockedImGui_; }

private:
	std::vector<std::string> leftDockWindows_;
	std::vector<std::string> rightDockWindows_;
	std::vector<std::string> bottomDockWindows_;
	bool enableDockedImGui_ = true;

private:
	// シーンマネージャー (借りてくる)
	SceneManager* sceneManager_ = nullptr;

	// ライトの初期化
	std::unique_ptr<Light> light = std::make_unique<Light>();

	// FPS計測用の変数
	std::chrono::steady_clock::time_point lastFrameTime_ = std::chrono::steady_clock::now();
	float fps_ = 0.0f;

};


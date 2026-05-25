#pragma once
#include "Light.h"
#include <chrono>
#include "PostEffect.h"
#include "SceneController.h"

// 前方宣言
class SceneManager;

/// <summary>
/// シーンの基底クラス
/// 
/// 毎フレームのライフサイクル (SceneManager から呼ばれる順):
///   1. UpdateCamera() : カメラ位置/行列を最新にする
///                       (他のオブジェクトがカメラ行列を参照する前に呼ぶ)
///   2. Update()       : ゲームロジック (キャラ移動、AI、アニメ等)
///   3. LateUpdate()   : 後処理 (カメラ追従、シェイク、カメラエフェクト等)
/// 
/// この3段階分離は UE5 / Unity と同じ設計で、
/// - スカイボックスが初期フレームで崩れる問題を防ぐ
/// - カメラ追従が1フレーム遅れる問題を防ぐ
/// - DebugCamera などの "描画前にカメラを差し替える" 機能を綺麗に実現できる
/// 
/// 派生シーンは Update は必ず実装する。UpdateCamera/LateUpdate は必要に応じてoverride。
/// </summary>
class BaseScene
{
public:
	//------メンバ関数------

	BaseScene() {
		AddLeftDockWindow("FPS");
	}

	virtual ~BaseScene() = default;

	/// <summary>初期化 (シーン起動時に1回)</summary>
	virtual void Initialize() = 0;

	/// <summary>終了 (シーン終了時に1回)</summary>
	virtual void Finalize() = 0;

	/// <summary>
	/// カメラ更新フェーズ (フレームの最初に呼ばれる)
	/// 
	/// ここでカメラの位置・角度・ViewMatrix/ProjectionMatrix を最新にする。
	/// 派生シーンで、FollowCamera など「プレイヤー位置に依存しないカメラの基本処理」を実装する。
	/// 
	/// デフォルト実装は空。カメラを持たないシーンはoverride不要。
	/// </summary>
	virtual void UpdateCamera() {}

	/// <summary>ゲームロジックの更新 (2番目)</summary>
	virtual void Update() = 0;

	/// <summary>
	/// 後処理フェーズ (Update後に呼ばれる)
	/// 
	/// ここで「更新されたキャラ位置に追従するカメラ」「カメラエフェクト」等を実装する。
	/// デフォルト実装は空。必要なシーンのみoverride。
	/// </summary>
	virtual void LateUpdate() {}

	/// <summary>背景描画</summary>
	virtual void BackGroundDraw() = 0;

	/// <summary>描画</summary>
	virtual void Draw() = 0;

	/// <summary>前景描画</summary>
	virtual void ForeGroundDraw() = 0;

	/// <summary>デバッグ用 (Imguiなど)</summary>
	virtual void Debug() = 0;

	/// <summary>シーンマネージャーをシーンに貸し出すためのSetter</summary>
	virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

	/// <summary>
	/// このシーン中にマウスカーソルを表示するかどうか。
	/// デフォルトは true (タイトル/メニュー/エディタ等の汎用シーン向け)。
	/// ゲームプレイのようにカーソルを隠したいシーンで override する。
	/// </summary>
	virtual bool ShouldShowCursor() const { return true; }

	/// <summary>
	/// このシーン中にマウスカーソルをウィンドウ内に閉じ込めるか。
	/// デフォルトは false。FPS/TPS等のゲームプレイで true に override する。
	/// </summary>
	virtual bool ShouldConfineCursor() const { return false; }

	/// <summary>Lightのゲッター</summary>
	Light* GetLight() const {
		return light.get();
	}

	void ShowFPS() {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<float> deltaTime = now - lastFrameTime_;
		lastFrameTime_ = now;
		fps_ = 1.0f / deltaTime.count();

		ImGui::Begin("FPS Display");
		ImGui::Text("Current FPS: %.2f", fps_);
		ImGui::End();
	}

public:
	// Dock候補登録用
	const std::vector<std::string>& GetLeftDockWindows() const { return leftDockWindows_; }
	const std::vector<std::string>& GetRightDockWindows() const { return rightDockWindows_; }
	const std::vector<std::string>& GetBottomDockWindows() const { return bottomDockWindows_; }

	void AddLeftDockWindow(const std::string& name) { leftDockWindows_.push_back(name); }
	void AddRightDockWindow(const std::string& name) { rightDockWindows_.push_back(name); }
	void AddBottomDockWindow(const std::string& name) { bottomDockWindows_.push_back(name); }

	void SetEnableDockedImGui(bool enable) { enableDockedImGui_ = enable; }
	bool IsDockedImGuiEnabled() const { return enableDockedImGui_; }

protected:
	// 派生シーンから SceneManager を参照したい時に使う (CursorService等の注入されたサービスへの橋渡し)
	SceneManager* GetSceneManager() const { return sceneManager_; }

private:
	std::vector<std::string> leftDockWindows_;
	std::vector<std::string> rightDockWindows_;
	std::vector<std::string> bottomDockWindows_;
	bool enableDockedImGui_ = true;

private:
	SceneManager* sceneManager_ = nullptr;
	std::unique_ptr<Light> light = std::make_unique<Light>();
	std::chrono::steady_clock::time_point lastFrameTime_ = std::chrono::steady_clock::now();
	float fps_ = 0.0f;
};
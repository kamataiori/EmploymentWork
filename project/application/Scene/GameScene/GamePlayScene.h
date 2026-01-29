#pragma once
#include <Vector2.h>
#include <vector>
#include <memory>
#include <Sprite.h>
#include <Object3d.h>
#include "BaseScene.h"
#include "Audio.h"
#include "Light.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "DrawLine.h"
#include "CollisionManager.h"
#include <Enemy/Enemy.h>
#include <FollowCamera.h>
#include "Camera/CameraEffectController.h"
#include "SkyBox.h"

class GamePlayScene : public BaseScene
{
public:
	//------メンバ関数------

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
	/// 背景描画
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景描画
	/// </summary>
	void ForeGroundDraw() override;

	void Debug() override;

	/// <summary>
	/// camera1のセッター
	/// </summary>
	void SetCamera1(std::unique_ptr<Camera> newCamera)
	{
		camera1 = std::move(newCamera);
	}

	/// <summary>
	/// camera1のゲッター
	/// </summary>
	Camera* GetCamera1() const
	{
		return camera1.get();
	}

	/// <summary>
	/// 衝突判定と応答
	/// </summary>
	void CheckAllCollisions();


private:

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;

	// カメラ演出用コントローラ
	std::unique_ptr<CameraEffectController> cameraEffect_;
	// フォローカメラを止めるかどうか
	bool followCameraLocked_ = false;
	// 撃破演出用：ズームを何秒後に開始するかのタイマー
	float defeatZoomTimer_ = -1.0f;   // < 0 なら未使用
	bool  defeatZoomStarted_ = false; // ズーム開始済みかどうか
	bool slowMotionStarted_ = false;  // スローモーションの開始時間
	// ズームが「進行中」かどうか＆残り時間
	bool  zoomActive_ = false;
	float zoomTimer_ = 0.0f;
	float zoomDuration_ = 0.0f;   // ズームの総時間を保存

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;

	std::unique_ptr<Object3d> sky;

	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;
	bool enemyWasDead_ = false;   // 前フレームの死亡状態

	std::unique_ptr<CollisionManager> collisionManager_;

	std::unique_ptr<SceneController> stage_;

	std::unique_ptr<Sprite> ex;


	// =========================
	// Pause（ポーズ）関連
	// =========================
	bool isPaused_ = false;

	// ポーズ画面のスプライト
	std::unique_ptr<Sprite> pauseBlack_;    // Black.png（半透明背景）
	std::unique_ptr<Sprite> pauseMenu_;     // menu.png (256x128)
	std::unique_ptr<Sprite> pauseOpe_;      // ope.png  (256x64)
	std::unique_ptr<Sprite> pauseBackTitle_; // backTitle.png (256x64)
	bool escLock_ = false;

	// ポーズ開始/終了
	void EnterPause();
	void ExitPause();

	// ポーズUI更新（必要なときだけUpdateする）
	void UpdatePauseSprites();

	// ===============
// Pause UI 追加
// ===============
	std::unique_ptr<Sprite> pauseExp_;     // exp.png

	bool pauseShowOptions_ = true;         // true: ope/backTitle表示, false: exp表示
	Vector2 opeBaseSize_{ 256.0f, 64.0f };
	Vector2 backBaseSize_{ 256.0f, 64.0f };
	Vector2 hoverSize_{ 288.0f, 72.0f };   // ホバー時の拡大（好みで調整）

	// マウス判定
	bool HitTestSprite(const Sprite* sp, const POINT& mouse) const;

	// ポーズ中のマウスUI処理
	void UpdatePauseMouseUI();

	// ポーズ画面内の状態
	enum class PauseView {
		Menu,     // menu + ope + backTitle
		Explain,  // menu + exp
	};

	PauseView pauseView_ = PauseView::Menu;

	// esc表示
	std::unique_ptr<Sprite> pauseEsc_;
	Vector2 escSize_{ 128.0f, 64.0f };
	Vector2 escBaseSize_{ 128.0f, 64.0f };
	Vector2 escHoverSize_{ 144.0f, 72.0f }; // 好みで調整


	void HandlePauseBack(); // ESC/escクリック共通

	// ゲーム中に表示するESCヒント（操作不可）
	std::unique_ptr<Sprite> escHint_;
	Vector2 escHintSize_{ 128.0f, 64.0f };

	// ポーズUIのアニメ状態
	enum class PauseAnimState {
		Entering,
		Idle,
		Exiting,
	};

	PauseAnimState pauseAnimState_ = PauseAnimState::Idle;

	// アニメ用タイマー（unscaledで進める）
	float pauseAnimTime_ = 0.0f;

	// アニメ設定
	float pauseEnterSec_ = 0.25f;   // 1個あたりの出る時間
	float pauseExitSec_ = 0.20f;   // 1個あたりの戻る時間
	float pauseStaggerSec_ = 0.08f; // 上から順の遅延

	// 目標位置（中央）と開始位置（左）
	Vector2 menuTargetPos_{};
	Vector2 opeTargetPos_{};
	Vector2 backTargetPos_{};

	Vector2 menuStartPos_{};
	Vector2 opeStartPos_{};
	Vector2 backStartPos_{};

	// アニメ更新
	void BeginPauseEnterAnim();
	void BeginPauseExitAnim();
	void UpdatePauseEnterExitAnim(float unscaledDt);
	void ApplySlideAnimToSprites(float t, bool entering);


};


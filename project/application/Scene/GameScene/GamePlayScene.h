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
#include "Player.h"
#include "DrawLine.h"
#include "CollisionManager.h"
#include <Enemy/Enemy.h>
#include <FollowCamera.h>
#include "Camera/CameraEffectController.h"
#include "SkyBox.h"
#include "engine/UI/UIManager.h"
#include "engine/UI/DamagePopupManager.h"
#include "PauseScreen.h"

class GamePlayScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;

	// 新しいライフサイクル (BaseScene)
	void UpdateCamera() override;   // カメラを最初に更新
	void Update() override;         // ゲームロジック
	void LateUpdate() override;     // 後処理(カメラ追従・カメラエフェクト)

	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;

	// Canvas合成方式を使用（UIを非エフェクト化・パーティクルを専用レイヤーへ分離）
	bool UsesCanvasCompositing() const override { return true; }
	void ParticleDraw() override;   // パーティクル専用Canvasへ
	void UIDraw() override;          // 合成後のバックバッファへ（エフェクト対象外）

	void Debug() override;
	void CheckAllCollisions();

	// マウスカーソル設定: ゲームプレイ中は非表示+ウィンドウ内に閉じ込める
	bool ShouldShowCursor() const override { return false; }
	bool ShouldConfineCursor() const override { return true; }

	void SetCamera1(std::unique_ptr<Camera> newCamera) { camera1 = std::move(newCamera); }
	Camera* GetCamera1() const { return camera1.get(); }

private:

	// ================================================
	// バトル開始イントロ演出
	// ================================================

	enum class IntroPhase
	{
		kCountdown, // 5→0 カウントダウン
		kStart,     // "START!" 表示
		kFinished,  // 演出終了
	};

	struct BattleIntroController
	{
		IntroPhase phase = IntroPhase::kCountdown;

		int   countdownNum = 5;
		float countdownTimer = 0.0f;
		float kCountPerSec = 1.0f;

		float startDisplayTimer = 0.0f;
		float kStartDisplaySec = 1.2f;

		bool isActive() const { return phase != IntroPhase::kFinished; }

		void Reset()
		{
			phase = IntroPhase::kCountdown;
			countdownNum = 5;
			countdownTimer = 0.0f;
			startDisplayTimer = 0.0f;
		}
	};

	BattleIntroController intro_;

	static constexpr int kCountMax = 5;
	std::array<std::unique_ptr<Sprite>, kCountMax + 1> countSprites_;
	std::unique_ptr<Sprite> startSprite_;

	void UpdateIntro(float dt);
	void DrawIntroUI();

	// Bloom調整用（ImGui）
	float bloomThreshold_ = 0.8f;
	float bloomIntensity_ = 1.2f;
	int   bloomIterations_ = 5;

	// ================================================

	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;

	std::unique_ptr<CameraEffectController> cameraEffect_;
	bool  followCameraLocked_ = false;
	float defeatZoomTimer_ = -1.0f;
	bool  defeatZoomStarted_ = false;
	bool  slowMotionStarted_ = false;
	bool  zoomActive_ = false;
	float zoomTimer_ = 0.0f;
	float zoomDuration_ = 0.0f;

	std::unique_ptr<SkyBox>   skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;
	std::unique_ptr<Object3d> sky;

	std::unique_ptr<Player>   player_;
	std::unique_ptr<Enemy>    enemy_;
	bool enemyWasDead_ = false;

	std::unique_ptr<CollisionManager> collisionManager_;
	std::unique_ptr<SceneController>  stage_;
	std::unique_ptr<Sprite>           ex;
	std::unique_ptr<UIManager>        uiManager_;

	// 敵への与ダメージ数値ポップアップ（敵へ注入する）
	std::unique_ptr<DamagePopupManager> damagePopup_;

	// PauseScreen の所有は uiManager_。こちらは状態監視用の非所有参照。
	PauseScreen* pauseScreenRef_ = nullptr;
	bool wasPausedLastFrame_ = false;

	// ポーズ状態の変化を見てカーソルを切替える
	void SyncCursorWithPauseState();

	bool  gameOverStarted_ = false;
	float vignetteTimer_ = 0.0f;
	const float kVignetteDuration_ = 2.0f;
	const float kVignetteScale_ = 0.3f;
	const float kVignettePower_ = 3.0f;
};
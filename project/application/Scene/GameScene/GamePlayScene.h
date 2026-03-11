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
#include "engine/UI/UIManager.h"
#include "PauseScreen.h"

class GamePlayScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;
	void Debug() override;
	void CheckAllCollisions();

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

		// ---- Countdown ----
		int   countdownNum = 5;
		float countdownTimer = 0.0f;
		float kCountPerSec = 1.0f; // 1カウント減らす間隔（秒）

		// ---- Start ----
		float startDisplayTimer = 0.0f;
		float kStartDisplaySec = 1.2f; // "START!" 表示時間（秒）

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

	// カウントダウン用スプライト [0]="0" 〜 [5]="5"
	static constexpr int kCountMax = 5;
	std::array<std::unique_ptr<Sprite>, kCountMax + 1> countSprites_;
	std::unique_ptr<Sprite> startSprite_;

	void UpdateIntro(float dt);
	void DrawIntroUI();

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
};
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


	std::unique_ptr<UIManager> uiManager_;
};


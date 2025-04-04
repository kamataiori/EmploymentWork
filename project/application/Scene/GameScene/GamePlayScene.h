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
#include <Enemy.h>

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
	void CheckAllColisions();

private:

	float offsetX = 100.0f;  //各スプライトのX座標をずらすオフセット値
	float initialX = 100.0f; //初期X座標

	// Spriteの初期化
	// 小さく描画するための初期スケールを設定
	Vector2 initialSize = { 80.0f,80.0f };
	std::vector<std::unique_ptr<Sprite>> sprites;

	std::unique_ptr<Sprite> monsterBall = std::make_unique<Sprite>();

	// 3Dオブジェクトの初期化
	std::unique_ptr<Object3d> plane = nullptr;
	std::unique_ptr<Object3d> axis = nullptr;

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<Camera> camera2 = std::make_unique<Camera>();
	bool cameraFlag = false;  //ImGuiで制御するカメラの切り替えフラグ

	std::unique_ptr<Audio> audio = std::make_unique<Audio>();
	Audio::SoundData sound = {};
	bool isAudio = false;

	Vector2 MonsterPosition;


	std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> emitters;


	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<CollisionManager> collisionMAnager_;

	Cone cone;
};


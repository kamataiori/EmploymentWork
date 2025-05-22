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
#include <FollowCamera.h>
#include "SkyBox.h"

//inline constexpr const char* kWindowName_PlayerControl = "Player Control";
//inline constexpr const char* kWindowName_EnemyControl = "Enemy Control";
inline constexpr const char* kWindowName_MonsterControl = "Monster Control";

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
	/// 衝突判定と応答
	/// </summary>
	void CheckAllColisions();


private:

	//3Dカメラの初期化
	std::unique_ptr<FollowCamera> followCamera;
	std::unique_ptr<Camera> camera;
	
	std::unique_ptr<Player> player_;

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();

	std::unique_ptr<CollisionManager> collisionManager_;
};


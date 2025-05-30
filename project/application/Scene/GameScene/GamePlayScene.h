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

	void Explosion();


private:

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;

	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<CollisionManager> collisionMAnager_;



	std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();  // パーティクル管理
	std::unique_ptr<ParticleEmitter> emitter = nullptr;  // エミッター

	// 初期設定を保持してImGuiで操作できるようにする
	Transform emitterTransform = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	EmitterConfig config = { ShapeType::Plane, 200, 1.0f, false };
	std::string groupName = "editor";
	Vector4 particleColor = { 1.0f, 0.5f, 0.0f, 1.0f };  // 初期は白（完全不透明）
	Vector3 particleVelocity = { 0.0f, 0.0f, 0.0f };  // 初期速度
	float particleLifeTime = 1.5f;  // 寿命
	float particleStartTime = 0.0f; // 開始時間
	bool useRandomPosition = true;  // ランダム位置を使うか
	bool useBillboard = true;

	int blendModeIndex = 2;  // kBlendModeAdd に対応（仮にAddから開始）

	std::vector<std::string> textureFiles = {
	"Resources/circle.png",
	"Resources/circle2.png",
	"Resources/gradationLine.png",
	"Resources/image.png",
	"Resources/monsterBall.png",
	"Resources/particleTest.png",
	"Resources/uvChecker.png",
	"Resources/smoke.png",
	"Resources/fire.png",
	};
	int selectedTextureIndex = 0;

	std::unique_ptr<ParticleManager> planeParticle2 = std::make_unique<ParticleManager>();
	std::unique_ptr<ParticleEmitter> emitter2 = nullptr;
	std::string groupName2 = "editor2";


	std::unique_ptr<ParticleManager> ringParticle = std::make_unique<ParticleManager>();  // 爆発リング専用
	std::vector<std::unique_ptr<ParticleEmitter>> ringExplosions;  // 爆発用の複数リング
	std::string explosionGroupName = "ring";  // リング用パーティクルグループ名


	std::unique_ptr<ParticleManager> primitiveParticle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> primitiveEmitters;

	bool explosionEffectStart = false;

	bool hasExploded_ = false;


};


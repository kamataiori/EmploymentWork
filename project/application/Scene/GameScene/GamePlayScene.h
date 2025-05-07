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
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdlib>  // rand()
#include <ctime>    // time()


//inline constexpr const char* kWindowName_PlayerControl = "Player Control";
//inline constexpr const char* kWindowName_EnemyControl = "Enemy Control";
inline constexpr const char* kWindowName_MonsterControl = "Monster Control";

enum class HeelEffectState {
	Waiting,
	Growing,
	Inactive
};

struct HeelEffect {
	std::unique_ptr<Sprite> sprite;
	Vector2 position;
	float scale = 0.0f;
	float lifetime = 0.0f;
	float lifetimeLimit = 1.0f; // ← ランダム終了時間（秒）
	float respawnTimer = 0.0f;
	HeelEffectState state = HeelEffectState::Waiting;

};

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

	inline Vector2 Add(const Vector2& a, const Vector2& b) {
		return { a.x + b.x, a.y + b.y };
	}

	bool IsFarEnough(const Vector2& newPos, const std::vector<HeelEffect>& effects, float minDistance) {
		for (const auto& e : effects) {
			if (e.state == HeelEffectState::Growing) {
				float dx = newPos.x - e.position.x;
				float dy = newPos.y - e.position.y;
				if ((dx * dx + dy * dy) < (minDistance * minDistance)) {
					return false; // 近すぎる
				}
			}
		}
		return true;
	}



private:

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<Camera> camera2 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;
	bool cameraFlag = false;  //ImGuiで制御するカメラの切り替えフラグ

	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<Sprite> heel_;
	//const int kHeelSpriteCount = 8;
	//const float kHeelRadius = 300.0f;  // 画面中心からの距離（半径）
	// heelアイコン複数表示用
	// heelアイコンの数と表示範囲
	//const int kHeelSpriteCount = 8;
	/*const float kHeelMinRadius = 200.0f;
	const float kHeelMaxRadius = 320.0f;*/

	// スプライトリストと制御フラグ
	std::vector<std::unique_ptr<Sprite>> heels_;
	bool isHealing = false;
	bool healingEffectInitialized = false;


	std::vector<HeelEffect> heelEffects_;
	const int kHeelSpriteCount = 8;
	const float kHeelMinRadius = 400.0f;
	const float kHeelMaxRadius = 700.0f;
	const float kHeelGrowSpeed = 0.01f;
	const float kHeelMaxScale = 1.0f;
	const float kHeelRespawnTime = 0.5f;  // 秒ごとに再表示
	float heelRespawnTimer_ = 0.0f;


	std::unique_ptr<Object3d> skydome_;

	std::unique_ptr<CollisionManager> collisionMAnager_;

};


#pragma once
#include "ObjectBase.h"
#include <CollisionTypeIdDef.h>

//======================================================
// MinionEnemy（雑魚敵）
//------------------------------------------------------
// ・ボスの召喚攻撃で出現
// ・地面の下から上昇して出現（Spawn演出）
// ・出現後、プレイヤーに向かって直進
// ・体当たりでダメージを与える
// ・プレイヤーの攻撃でHPが0になったら消える
//======================================================
class MinionEnemy : public ObjectBase
{
public:
	MinionEnemy(BaseScene* scene);
	~MinionEnemy() override = default;

	void Initialize() override {}

	// 実際の初期化（出現位置とターゲットを指定）
	void InitializeMinion(
		const Vector3& spawnPos,
		const Transform* targetTransform
	);

	void Update() override;

	void BackGroundDraw() override {}
	void Draw() override;
	void ForeGroundDraw() override {}
	void ParticleDraw() override {}
	void AnimationDraw() override {}

	void OnCollision() override {}
	void OnCollision(const CollisionInfo& info) override;

	bool IsDead() const { return isDead_; }

	void SetCamera(Camera* camera) override;
	void SetTargetTransform(const Transform* t) { targetTransform_ = t; }

private:
	enum class Phase {
		Spawn,   // 地面の下から上昇中
		Chase,   // プレイヤーに向かって追跡中
	};

	Phase phase_ = Phase::Spawn;

	const Transform* targetTransform_ = nullptr;

	bool isDead_ = false;

	// HP
	int hp_ = 1;
	static constexpr int kMaxHP_ = 1;

	// 出現演出
	Vector3 surfacePos_{};          // 地面の高さ（目標位置）
	float spawnDepth_ = 3.0f;       // 地面からどれだけ下に埋まった状態で始まるか
	float riseSpeed_ = 4.0f;        // 上昇速度

	// 移動
	float moveSpeed_ = 8.0f;
	float turnLerp_ = 0.1f;

	// コライダー
	float radius_ = 0.8f;

	// 寿命
	float lifeTimer_ = 0.0f;
	static constexpr float kMaxLifeTime_ = 15.0f;
};
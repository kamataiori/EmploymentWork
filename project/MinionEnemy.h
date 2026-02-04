#pragma once
#include "ObjectBase.h"

//==============================================
// MinionEnemy
// HP半分以下で召喚される雑魚敵
//==============================================
class MinionEnemy : public ObjectBase
{
public:
	explicit MinionEnemy(BaseScene* baseScene);
	~MinionEnemy() override = default;

	//==============================
	// ObjectBase 必須override
	//==============================
	void Initialize() override;        // 初期化
	void Update() override;            // 更新
	void BackGroundDraw() override {}  // 使わないので空
	void Draw() override;              // 描画
	void ForeGroundDraw() override {}  // 使わないので空
	void ParticleDraw() override {}    // 使わないので空
	void AnimationDraw() override {}   // 使わないので空

	void OnCollision() override {}     // 使わないので空
	void OnCollision(const CollisionInfo& info) override { (void)info; } // 必要なら受ける
	void SetCamera(Camera* camera) override; // Enemy同様にカメラをセット

	//==============================
	// Minion設定
	//==============================
	void SetTargetTransform(const Transform* target);
	void SetSpawnPositionXZ(const Vector3& posXZ);

	void SetRiseParams(float startY, float endY, float speed);
	void SetReadyDuration(float sec);
	void SetChaseParams(float speed, float turnLerp);

private:
	enum class State
	{
		Rising,
		Ready,
		Chasing
	};

	void UpdateRising(float dt);
	void UpdateReady(float dt);
	void UpdateChasing(float dt);

	static float WrapDeltaRad(float a);
	static float LerpAngleRad(float from, float to, float t);

private:
	const Transform* target_ = nullptr;
	State state_ = State::Rising;

	float riseStartY_ = -1.0f;
	float riseEndY_ = 2.0f;
	float riseSpeed_ = 2.5f;

	float readyTimer_ = 0.0f;
	float readyDuration_ = 0.35f;

	float chaseSpeed_ = 5.0f;
	float turnLerp_ = 0.15f;
};

#pragma once
#include "ObjectBase.h"
#include <string>

class Player;

class Sword : public ObjectBase
{
public:
	Sword(BaseScene* scene) : ObjectBase(scene) {}

	void Initialize() override;
	void Update() override;

	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;
	void ParticleDraw() override;
	void AnimationDraw() override;
	void OnCollision() override;

	void AttachTo(Object3d* ownerObj, const std::string& jointName);

	void SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s);

	void SetHitEnabled(bool enabled) { hitEnabled_ = enabled; }
	// 攻撃中で当たり判定が有効か（Scene のコライダー登録判定に使う）
	bool IsHitEnabled() const { return hitEnabled_; }

	MultiCollider* GetMultiCollider() const { return multiCollider_.get(); }

	// プレイヤーのTransformを参照して向き(yaw)を取る
	void SetPlayerTransform(const Transform* t) { playerTransform_ = t; }

private:
	Object3d* ownerObj_ = nullptr;
	std::string ownerJoint_;

	const Transform* playerTransform_ = nullptr;

	bool hitEnabled_ = false;
	bool isHit_ = false;

	// OBB調整値
	Vector3 hitOffset_ = { 0.0f, 0.0f, 8.5f };
	Vector3 hitHalfSize_ = { 0.7f, 0.8f, 2.2f };

	Vector3 defaultOffsetT_ = { -0.44f, -0.73f, -0.48f };
	Vector3 defaultOffsetR_ = { -0.56f, 4.95f, 0.0f };
	Vector3 defaultOffsetS_ = { 1.0f, 1.0f, 1.0f };
};
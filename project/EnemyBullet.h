#pragma once
#include "CharacterBase.h"
#include "SphereCollider.h"

class EnemyBullet : public CharacterBase, public SphereCollider
{
public:
	EnemyBullet(BaseScene* baseScene);

	~EnemyBullet();

	// 初期化
	void Initialize() override;

	// 更新（移動処理）
	void Update() override;

	// 描画
	void Draw() override;

	void OnCollision() override;

	// 速度設定
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	const Vector3& GetVelocity() const { return velocity_; }

	// 初期位置設定（オプション）
	void SetTranslate(const Vector3& pos) { transform.translate = pos; }

	void SetCamera(Camera* camera) {
		camera_ = camera;
		object3d_->SetCamera(camera);
	}

	bool IsDead() const { return isDead_ || lifeTimer_ >= maxLifeTime_; }

private:
	Vector3 velocity_ = { 0.0f, 0.0f, 2.0f }; // デフォルト前進速度
	float lifeTimer_ = 0.0f;
	const float maxLifeTime_ = 4.0f;
	bool isDead_ = false;  // 弾が消えるかどうか
};


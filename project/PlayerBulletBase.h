// ===============================
// 共通挙動（移動・寿命・当たり属性）を持つ基底
// ===============================
#pragma once
#include "PlayerIBullet.h"
#include "SphereCollider.h"
#include "Object3d.h"
#include "CollisionTypeIdDef.h"
#include <memory>

class PlayerBulletBase : public PlayerIBullet, public SphereCollider {
public:
	PlayerBulletBase() : SphereCollider(sphere_) {}
	~PlayerBulletBase() override = default;


	void Initialize(BaseScene* scene) override;
	void Update() override;
	void Draw() override;


	void Shoot(const Vector3& pos, const Vector3& dir, float speed, float lifeSec) override;


	bool IsAlive() const override { return alive_; }


	void SetOwner(PlayerBase* owner) override { owner_ = owner; }


protected:
	// 派生クラスでモデル名を指定（空ならデバッグ球のみ）
	virtual const char* GetModelName() const { return nullptr; }


	// 派生が独自の毎フレーム処理を差し込みたい場合
	virtual void OnUpdate() {}


	// 派生が衝突時の挙動を入れる場合（敵に当たった、地形に当たった等）
	virtual void OnCollision() {}


protected:
	BaseScene* scene_ = nullptr;
	std::unique_ptr<Object3d> obj_;


	// 運動
	Vector3 pos_{};
	Vector3 dir_{}; // 正規化済み
	float speed_ = 0.0f; // フレーム当たりの移動量（エンジン準拠）


	// 寿命
	float lifeSec_ = 0.0f; // 残り寿命（秒）
	bool alive_ = false;


	// コリジョン半径
	Sphere sphere_{}; // baseのメンバ参照先


	// Δt（寿命管理用）
	static constexpr float kDt = 1.0f / 60.0f;


	// オーナ参照（必要に応じて使う）
	PlayerBase* owner_ = nullptr;
};


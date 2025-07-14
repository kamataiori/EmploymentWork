#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include <PlayerBullet.h>

// アニメーション名管理構造体
struct AnimationSet {
	std::string Death = "Death";
	std::string Idle = "Idle";
	std::string Idle_Attacking = "Idle_Attacking";
	std::string Idle_Weapon = "Idle_Weapon";
	std::string PickUp = "PickUp";
	std::string Punch = "Punch";
	std::string RecieveHit = "RecieveHit";
	std::string RecieveHit_2 = "RecieveHit_2";
	std::string Roll = "Roll";
	std::string Run = "Run";
	std::string Run_Weapon = "Run_Weapon";
	std::string Sword_Attack = "Sword_Attack";
	std::string Sword_AttackFast = "Sword_AttackFast";
	std::string Walk = "Walk";
};

struct MonkAnimationSet {
	std::string Attack = "Attack";
	std::string Attack2 = "Attack2";
	std::string Death = "Death";
	std::string Idle = "Idle";
	std::string Idle_Attacking = "Idle_Attacking";
	std::string PickUp = "PickUp";
	std::string RecieveHit = "RecieveHit";
	std::string RecieveHit_Attacking = "RecieveHit_Attacking";
	std::string Roll = "Roll";
	std::string Run = "Run";
	std::string Walk = "Walk";
};


class Player : public CharacterBase, public SphereCollider
{
public:

	Player(BaseScene* baseScene_) : CharacterBase(baseScene_),SphereCollider(sphere){}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void BulletDraw();

	void OnCollision() override;

	PlayerBullet* GetBullet() const {
		return bullet_.get();
	}

private:

	// 移動＆ジャンプを処理する関数
	void Move();

	// 弾処理まとめ関数
	void HandleBullet();

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

private:

	// 移動制御に関する構造体
	struct MoveControl {
		float speed = 0.2f;                    // 移動速度
		Vector3 direction{};                   // 入力から得た移動方向
		float dashSpeed = 1.0f;                // ダッシュ時の速度
		bool isDashing = false;                // ダッシュ中かどうか
		bool hasDashed_ = false;               // 1回だけダッシュ許可
		bool isDashKeyHeld_ = false;           // Bキーがまだ押されているかどうか
		const float kDashDuration = 0.5f;      // ダッシュ継続時間
		float dashTimer = 0.0f;                // 残りダッシュ時間
	}; 
	MoveControl move_;  // 移動制御

	// ジャンプに関するデータ構造体
	struct JumpControl {
		bool isJumping = false;               // ジャンプ中か
		float velocity = 0.0f;                // 上下速度
		int jumpCount = 0;                    // 現在ジャンプ回数
		const int kMaxJumpCount = 2;          // 最大ジャンプ回数
		bool canJump_ = true;                 // Spaceキーが離されたことを確認するフラグ
		const float kInitialVelocity = 0.45f; // 初速
		const float kGravity = 0.02f;         // 重力
		const float kGroundHeight = 0.0f;     // 地面高さ
	};
	// インスタンス
	JumpControl jump_;

	// 弾
	std::unique_ptr<PlayerBullet> bullet_; // 今回は1発だけ

	AnimationSet animation_; // アニメーション名セット
	MonkAnimationSet MonkAnimation_;
	std::string currentAnimationName_;



	bool isAttacking_ = false;
	float attackAnimTimer_ = 0.0f;
	const float kAttackAnimDuration_ = 0.8f; // 攻撃アニメの長さに応じて調整


};


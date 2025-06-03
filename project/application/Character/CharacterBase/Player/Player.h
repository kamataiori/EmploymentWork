#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include <PlayerBullet.h>

class Player : public CharacterBase, public SphereCollider
{
public:

	Player(BaseScene* baseScene_) : CharacterBase(baseScene_),SphereCollider(sphere){}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void SetCamera(Camera* camera) override;


	void DrawParticle();

	PlayerBullet* GetBullet() const {
		return bullet_.get();
	}


private:

	// 移動＆ジャンプを処理する関数
	void Move();

	// 弾処理まとめ関数
	void HandleBullet();

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

	// 埃パーティクル用
	std::unique_ptr<ParticleManager> dustParticle_ = nullptr;
	std::unique_ptr<ParticleEmitter> dustEmitter_ = nullptr;

	Transform emitterTransform = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	EmitterConfig config = { ShapeType::Plane, 200, 1.0f, false };
	std::string groupName = "editor";
	Vector4 particleColor = { 1.0f, 0.5f, 0.0f, 1.0f };  // 初期は白（完全不透明）
	Vector3 particleVelocity = { 0.0f, 0.0f, 0.0f };  // 初期速度
	float particleLifeTime = 1.5f;  // 寿命
	float particleStartTime = 0.0f; // 開始時間
	bool useRandomPosition = true;  // ランダム位置を使うか
	bool useBillboard = true;

};


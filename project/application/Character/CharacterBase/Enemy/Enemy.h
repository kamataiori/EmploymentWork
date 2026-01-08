#pragma once
#include "ObjectBase.h"
#include "MultiCollider.h"
#include <memory>

class EnemyAIController;

struct SkeltonAnimationSet {
	std::string Death = "Death";
	std::string Duck = "Duck";
	std::string HitReact = "HitReact";
	std::string Idle = "Idle";
	std::string Jump = "Jump";
	std::string Jump_Idle = "Jump_Idle";
	std::string Jump_Idlea = "Jump_Idlea";
	std::string Jump_Land = "Jump_Land";
	std::string No = "No";
	std::string Punch = "Punch";
	std::string Run = "Run";
	std::string Sword = "Sword";
	std::string Walk = "Walk";
	std::string Wave = "Wave";
	std::string Yes = "Yes";
};

class Enemy : public ObjectBase
{
public:

	Enemy(BaseScene* baseScene_);
	~Enemy();

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 背景スプライト処理
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 通常のObject専用の描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景スプライト処理
	/// </summary>
	void ForeGroundDraw() override;

	/// <summary>
	/// Skiningのモデル専用の描画処理
	/// </summary>
	void AnimationDraw() override;

	/// <summary>
	/// パーティクル専用の描画処理
	/// </summary>
	void ParticleDraw() override;

	/// <summary>
	/// 当たり判定の呼出し
	/// </summary>
	void OnCollision() override;

	/// <summary>
	/// カメラをセット
	/// </summary>
	void SetCamera(Camera* camera) override;

	Camera* GetCamera() const { return camera_; }

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

	// 追尾ターゲット（Player）を渡してください（例：enemy->SetTargetTransform(&player->Get()->transform);）
	void SetTargetTransform(const Transform* t) { target_ = t; }

	bool IsDead() const { return isDead_; }

	// BT側が target_ を参照できるように getter を追加
	const Transform* GetTargetTransform() const { return target_; }

	// BT側がアニメ名を参照できるように getter を追加
	const SkeltonAnimationSet& GetAnimSet() const { return animation_; }

private:

	SkeltonAnimationSet animation_; // アニメーション名セット
	std::string currentAnimationName_;

	Vector3 obbSize_ = { 1.78f, 2.2f, 1.0f };
	Vector3 colliderOffset_ = { 0.0f, 1.8f, 0.0f }; // 原点(足元)→胴体中心へのオフセット
	Vector3 colliderCenter_ = {};                   // 実際のOBB中心

	float hitReactTimer_ = 0.0f;
	static inline constexpr float kHitReactDuration_ = 0.2f; // 秒

	// AIはここに閉じ込める
	std::unique_ptr<EnemyAIController> aiController_;


	// ====== 追尾＆突進AI ======
	enum class RushState { Idle, Dash, Cooldown };
	RushState state_ = RushState::Idle;
	float stateTimer_ = 0.0f;

	// パラメータ
	float idleTime_ = 0.60f;  // プレイヤーへ向き直る時間
	float dashTime_ = 1.75f;  // 突進継続時間
	float cooldownTime_ = 4.70f;  // 硬直
	float dashSpeed_ = 0.35f;  // 突進速度（毎フレ加算）
	float turnLerp_ = 0.18f;  // Idle/Cooldown 中の向き直りスムージング0..1

	Vector3 dashDir_ = { 0,0,0 }; // ダッシュ開始時に確定（XZ）
	const Transform* target_ = nullptr; // Player の Transform を参照

	// HP
	int hp_ = 1000;                   // 現在HP
	const int kMaxHP_ = 1000;         // 最大HP
	const int kDamagePerHit_ = 10;   // 被弾時のダメージ量

	// === HPバー表示用 ===
	std::unique_ptr<Sprite> hpBarBG_;    // 背景（薄い色）
	std::unique_ptr<Sprite> hpBarFill_;  // 本体（現在HPに応じて伸縮）
	// 配置とサイズ（1280x720想定）
	float hpBarMaxWidth_ = 420.0f;       // 最大幅
	float hpBarHeight_ = 20.0f;        // 高さ
	float hpBarTop_ = 20.0f;        // 画面上端からのオフセット

	bool isDead_ = false;        // 死亡状態かどうか
	float deathTimer_ = 0.0f;    // 死亡経過時間
	const float kDeathToTitleDelay_ = 8.5f; // タイトルへ戻るまでの秒数

	// 死亡時のスケール（ここから 0 まで縮小していく）
	Vector3 deathStartScale_{ 1.0f, 1.0f, 1.0f };
	// 爆破エフェクトをすでに出したかどうか
	bool hasSpawnedExplosion_ = false;

	Vector3 explosionPos = {};

	// 死亡エフェクト用パーティクル
	Transform deathParticleTransform_{}; // Emit位置用

	std::unique_ptr<ParticleManager> deathSystem_ = std::make_unique<ParticleManager>();

};


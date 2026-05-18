#pragma once
#include "ObjectBase.h"
#include "MultiCollider.h"
#include <memory>
#include <list>
#include <MinionEnemy.h>

class EnemyAIController;
class EnemyDropBullet;
class EnemySplitBullet;
class UIManager;
class EnemyStateManager;

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
	/// 当たり判定の呼出し
	/// </summary>
	void OnCollision(const CollisionInfo& info) override;

	/// <summary>
	/// カメラをセット
	/// </summary>
	void SetCamera(Camera* camera) override;

	/// <summary>
	/// イントロ演出専用：AI/弾を動かさずに見た目だけ更新する
	/// </summary>
	void UpdateVisual();

	// 初期位置
	const Vector3& GetHomePosition() const { return homePosition_; }

	Camera* GetCamera() const { return camera_; }

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

	// 追尾ターゲット（Player）を渡してください
	void SetTargetTransform(const Transform* t) { target_ = t; }

	bool IsDead() const { return isDead_; }

	// HPフェーズ
	enum class EnemyPhase { Phase1, Phase2, Phase3 };
	float GetHPRatio() const { return static_cast<float>(hp_) / static_cast<float>(kMaxHP_); }
	// 怒り状態（HP50%以下）
	bool IsAngry() const { return GetHPRatio() <= 0.5f; }

	EnemyPhase GetPhase() const {
		float r = GetHPRatio();
		if (r > 0.5f) return EnemyPhase::Phase1;
		if (r > 0.25f) return EnemyPhase::Phase2;
		return EnemyPhase::Phase3;
	}

	// 怒り時：分裂弾を多発（8発）
	void SpawnSplitBurstAngry(const Vector3& playerPos);

	// 大弾を投げる（ThrowBigBulletState から呼ばれる）
	void SpawnBigBullet(const Vector3& targetPos);

	// BT側が target_ を参照できるように getter を追加
	const Transform* GetTargetTransform() const { return target_; }

	// BT側がアニメ名を参照できるように getter を追加
	const SkeltonAnimationSet& GetAnimSet() const { return animation_; }

	// 被弾リアクション中か（BTがアニメを上書きしないため）
	bool IsHitReact() const { return hitReactTimer_ > 0.0f; }

	// Dash後の1パターン用：落下弾を生成
	void SpawnDropBullet(const Vector3& targetPos);

	// 4発バースト（上昇→分裂→直線発射）
	void SpawnSplitBurstToPlayer(const Vector3& playerPos);

	// 雑魚敵を1体召喚（SummonMinionState から呼ばれる）
	void SpawnMinion(const Vector3& spawnPos);

	// リストの公開（Scene側でコライダー登録用）
	const std::list<std::unique_ptr<MinionEnemy>>& GetMinions() const { return minions_; }

	// Scene側でコライダー登録するために公開
	const std::list<std::unique_ptr<EnemyDropBullet>>& GetDropBullets() const { return dropBullets_; }
	const std::list<std::unique_ptr<EnemySplitBullet>>& GetSplitBullets() const { return splitBullets_; }

	// ステートマネージャへのアクセス（BT の ExecuteStateLeaf が使う）
	EnemyStateManager* GetStateManager() { return stateManager_.get(); }
	EnemyAIController* GetAIController() const { return aiController_.get(); }

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
	const Transform* target_ = nullptr; // Player の Transform を参照

	std::list<std::unique_ptr<EnemyDropBullet>> dropBullets_;
	std::list<std::unique_ptr<EnemySplitBullet>> splitBullets_;

	// 生成時（Initialize時）の位置を保存しておく
	Vector3 homePosition_{};

	// HP
	int hp_ = 300;                   // 現在HP
	const int kMaxHP_ = 300;         // 最大HP
	const int kDamagePerHit_ = 30;   // 被弾時のダメージ量

	// === HPバー表示用 ===
	std::unique_ptr<UIManager> uiManager_;

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

	// ステートマネージャ（BT が選んだ攻撃の実行を管理）
	std::unique_ptr<EnemyStateManager> stateManager_;

	// 雑魚敵リスト
	std::list<std::unique_ptr<MinionEnemy>> minions_;

	// 現在突進シーケンスを実行中の雑魚敵（1体ずつ順番に動かす）
	MinionEnemy* activeMinion_ = nullptr;

	// SplitBullet の順次発射制御
	float splitFireTimer_ = 0.0f;
	float splitFireInterval_ = 0.3f;
	bool  splitFireActive_ = false;

	void UpdateSplitBulletFiring(float dt);

	// 雑魚敵を1体ずつ順番に突進させるコーディネーター
	void UpdateMinionCoordinator();
};
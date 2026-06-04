#pragma once
#include "ObjectBase.h"
#include <CollisionTypeIdDef.h>

class EnemySplitBullet : public ObjectBase
{
public:
	EnemySplitBullet(BaseScene* scene) : ObjectBase(scene) {}
	~EnemySplitBullet() override = default;

	// ObjectBase の pure virtual を満たす
	void Initialize() override {}

	// 実際の初期化
	void InitializeBurst(
		const Vector3& startPos,
		const Vector3& lockPlayerPos,
		float riseHeight,     // どこまで上がるか
		float riseSpeed,      // 上昇速度
		float splitRadius,    // 分裂時の4点の半径
		float shotSpeed       // 発射速度
	);

	void Update() override;

	void BackGroundDraw() override {}
	void Draw() override;              // デバッグ球のみ
	void ForeGroundDraw() override {}
	void ParticleDraw() override;
	void AnimationDraw() override {}

	void OnCollision() override { isDead_ = true; }
	void OnCollision(const CollisionInfo& info) override;

	bool IsDead() const { return isDead_; }

	void SetIndex(int i) { index_ = i; }

	// 分裂移動が完了して待機中かどうか（Enemy側から確認用）
	bool IsReadyToShot() const { return phase_ == Phase::SplitWait; }

	// Enemy側から発射許可を出す
	void Fire();

	// カメラ設定をパーティクルにも渡す
	void SetCamera(Camera* camera) override;

private:
	enum class Phase {
		Rise,           // 上昇
		RiseWait,       // 上昇後の待機
		SplitMove,      // 四方向に分裂移動
		SplitWait,      // 分裂完了 → 発射許可を待つ
		Shot            // プレイヤーに向けて発射
	};
	Phase phase_ = Phase::Rise;

	// 0..3 どの弾か（分裂後の配置に使う）
	int index_ = 0;

	Vector3 startPos_{};
	Vector3 lockPlayerPos_{}; // 分裂後に向ける先（ロック）

	float radius_ = 0.8f;

	float riseHeight_ = 0.01f;
	float riseSpeed_ = 80.0f;

	float splitRadius_ = 10.0f;
	float splitMoveSpeed_ = 20.0f;

	float shotSpeed_ = 50.0f;
	Vector3 shotDir_{ 0,0,1 };

	// 分裂後の目的位置（4点）
	Vector3 splitTargetPos_{};

	bool isDead_ = false;

	// フェーズ間の待機タイマー
	float phaseTimer_ = 0.0f;

	// 上昇後に止まる時間（調整用）
	float riseWaitDuration_ = 0.5f;

	//==========================
	// Particle
	//==========================
	std::unique_ptr<ParticleManager> particleSystem_;
	Transform particleTransform_{};

	float emitTimer_ = 0.0f;
	float emitInterval_ = 0.03f;

	std::string particleSystemName_ = "EnemyBullet";
	bool hasEmitted_ = false;
};
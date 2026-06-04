#pragma once
#include "ObjectBase.h"
#include "ITarget.h"
#include <CollisionTypeIdDef.h>
#include "Sprite.h"
#include "ParticleManager.h"
#include <array>
#include <memory>

class IDamagePopupSink;

//======================================================
// MinionEnemy（雑魚敵）
//------------------------------------------------------
// ・ボスの召喚攻撃で出現
// ・地面の下から上昇して出現（Spawn演出）
// ・コーディネーター(Enemy)に選ばれた1体ずつが
//   プレイヤーへ突進 → 上昇 → てっぺんで高速回転 → 急降下(ドッスン) → 衝撃波
// ・自分の番でない間は完全静止で待機
//======================================================
class MinionEnemy : public ObjectBase, public ITarget
{
public:
	MinionEnemy(BaseScene* scene);
	~MinionEnemy() override = default;

	void Initialize() override {}

	// 実際の初期化（出現位置を指定）
	void InitializeMinion(const Vector3& spawnPos);

	void Update() override;

	void BackGroundDraw() override {}
	void Draw() override;
	void ForeGroundDraw() override;
	void ParticleDraw() override;
	void AnimationDraw() override {}

	void OnCollision() override {}
	void OnCollision(const CollisionInfo& info) override;

	bool IsDead() const { return isDead_; }

	//=== ITarget（プレイヤーの攻撃対象としてのインターフェイス）===
	// 撃破演出(Phase::Hit)に入った時点で対象から外す（多重ヒット・再突進を防ぐ）。
	bool IsAlive() const override { return !isDead_ && phase_ != Phase::Hit && phase_ != Phase::Explode; }
	Vector3 GetTargetCenter() const override { return transform.translate + Vector3{ 0.0f, 1.0f, 0.0f }; }
	void ApplyDamage(int amount) override;

	void SetCamera(Camera* camera) override;

	//--- コーディネーター(Enemy)用インターフェイス ---
	// 出現演出が完了しているか
	bool IsSpawnFinished() const { return phase_ != Phase::Spawn; }
	// 突進～急降下シーケンスの実行中か
	bool IsAttacking() const { return attackActive_; }
	// 待機(Idle)中か
	bool IsIdle() const { return phase_ == Phase::Idle; }
	// 突進開始（targetPos = 突進先として取得したプレイヤー位置）
	void BeginAttack(const Vector3& targetPos);
	// このラウンドで既に突進し終えたか
	bool HasActed() const { return actedThisRound_; }
	void SetActed(bool v) { actedThisRound_ = v; }
	// 次のラウンドへ：行動済みフラグをリセット
	void ResetRound() { actedThisRound_ = false; }
	// 待機中にプレイヤーの方を向くための参照を渡す（毎フレーム更新でよい）
	void SetPlayerTarget(const Transform* t) { playerTarget_ = t; }

	// ダメージ数値ポップアップの注入口（Scene 所有。Enemy 経由で渡される）
	void SetDamagePopupSink(IDamagePopupSink* sink) { damageSink_ = sink; }

private:
	enum class Phase {
		Spawn,      // 地面の下から上昇中
		Idle,       // 待機（完全静止）
		Charge,     // プレイヤー位置へ突進
		Rise,       // 突進先で上昇
		SpinTop,    // てっぺんで停止して高速回転
		Slam,       // 急降下
		Shockwave,  // 着地直後の衝撃波
		Recover,    // 急降下後の硬直（無防備な反撃チャンス）
		Hit,        // 被弾ヒット演出（固まって小刻みシェイク → 爆発へ）
		Explode,    // シェイク後の爆発演出（パーティクル再生 → 消滅）
	};

	Phase phase_ = Phase::Spawn;

	bool isDead_ = false;
	bool attackActive_ = false;   // Charge～Shockwave 実行中
	bool actedThisRound_ = false; // コーディネーターが管理

	// HP（HP.png 1枚 = 1HP ピップ表示）
	static constexpr int kMaxHP_ = 5;
	static constexpr int kDamagePerHit_ = 2;               // プレイヤー攻撃1ヒットで減るHP量（5HPに対して3ヒットで撃破）
	int hp_ = kMaxHP_;

	// ===== 個体ごとの頭上HPピップ（HP.pngを実寸で横に並べる） =====
	std::array<std::unique_ptr<Sprite>, kMaxHP_> hpPips_;
	bool hpBarVisible_ = false;
	void UpdateHpBar();                                    // 毎フレーム：ピップ位置と表示可否を更新
	static constexpr float kHpBarHeadOffsetY_ = 2.2f;      // 立ち位置から頭上までのワールドYオフセット
	static constexpr float kHpPipWidth_ = 4.0f;            // HP.png 実寸の幅（拡大しない）
	static constexpr float kHpPipHeight_ = 8.0f;           // HP.png 実寸の高さ
	static constexpr float kHpPipSpacing_ = 1.0f;          // ピップ間のすき間（スクリーン単位）
	static constexpr float kHpBarMinClipW_ = 1e-5f;        // クリップ空間で背後判定する閾値
	// ピップに乗算する色（ユーザー指定の緑）。{1,1,1,1} ならテクスチャ色そのまま
	static constexpr Vector4 kHpPipColor_ = { 0.0f, 1.0f, 0.0f, 1.0f };

	// 出現演出
	Vector3 surfacePos_{};         // 立ち位置（出現の目標。地面＋groundOffsetY_）
	float groundY_ = 0.0f;         // 立ち位置のY（上昇/急降下の基準）
	float groundOffsetY_ = 1.0f;   // 地面からの持ち上げ量（足元を地面に合わせる）
	float spawnDepth_ = 3.0f;      // 立ち位置からどれだけ埋まって始まるか
	float spawnRiseSpeed_ = 4.0f;  // 出現上昇速度

	// 突進（Charge）
	Vector3 chargeTargetXZ_{};     // 突進先（BeginAttackで取得したプレイヤー位置）
	float chargeSpeed_ = 40.0f;    // 突進速度（一気に詰め寄る）
	float turnLerp_ = 0.35f;       // 向き補間
	float reachThreshold_ = 0.6f;  // 到達とみなす距離

	// 上昇（Rise：急降下準備）
	float liftSpeed_ = 20.0f;      // 上昇速度
	float riseHeight_ = 10.0f;     // 地面からの到達高さ

	// てっぺんでの高速回転（SpinTop）
	float spinSpeed_ = 42.0f;      // 回転速度(rad/s)
	float spinTurns_ = 4.0f;       // 回転する周回数
	float spinAccumulated_ = 0.0f; // 回転した累積角度(rad)

	// 急降下（Slam）
	float slamSpeed_ = 60.0f;      // 急降下速度

	// 衝撃波（Shockwave）
	float shockwaveRadius_ = 3.0f;   // 着地時の範囲ダメージ半径
	float shockwaveDuration_ = 0.25f;// 衝撃波の継続時間
	float shockwaveTimer_ = 0.0f;

	// 急降下後の硬直（Recover）：着地直後の無防備時間＝プレイヤーの反撃チャンス
	float recoverDuration_ = 0.7f;        // 硬直の長さ（秒）＝反撃チャンスの長さ
	float scaleRecoverDuration_ = 0.3f;   // 潰れスケールが通常へ戻るまでの時間（硬直より短く）
	float recoverTimer_ = 0.0f;
	Vector3 impactScale_ = { 1.3f, 0.6f, 1.3f }; // 着地で潰れるスケール（Recoverで通常へ戻す）

	// 生きた待機（Idle演出）：完全静止にせず、ホバー揺れ＋プレイヤー追従
	const Transform* playerTarget_ = nullptr; // プレイヤー参照（向き追従用）
	float idleTime_ = 0.0f;          // 待機演出の経過時間
	float idleBobAmplitude_ = 0.3f;  // ホバー上下動の振幅
	float idleBobSpeed_ = 3.0f;      // ホバー上下動の速さ(rad/s)
	float idleTurnLerp_ = 0.04f;     // プレイヤーへ向く補間の強さ

	// コライダー
	float radius_ = 0.8f;

	// 被弾ヒット演出（その場で固まり、小刻みにシェイクしてから消える）
	Vector3 hitBasePos_{};                            // シェイク中心となる基準位置
	float hitTimer_ = 0.0f;                           // ヒット演出の経過時間
	static constexpr float kHitDuration_ = 0.5f;     // 演出全体の長さ（秒）
	static constexpr float kHitShakeAmplitude_ = 0.30f; // シェイクの振幅（ワールド単位）
	static constexpr float kHitShakeFreqX_ = 60.0f;   // X方向のシェイク周波数(rad/s)
	static constexpr float kHitShakeFreqZRatio_ = 1.3f; // Z方向の周波数倍率（X基準）
	static constexpr float kHitShakeFreqYRatio_ = 0.9f; // Y方向の周波数倍率（X基準）
	static constexpr float kHitShakeYAttenuation_ = 0.5f; // Y方向シェイクの振幅減衰係数
	static constexpr float kHitShakePhaseZ_ = 1.7f;   // Z方向の初期位相（X とずらすため）
	static constexpr float kHitShakePhaseY_ = 0.5f;   // Y方向の初期位相

	// ダメージ数値ポップアップの注入口（Scene 所有。参照のみ）
	IDamagePopupSink* damageSink_ = nullptr;

	// 撃破時の爆発パーティクル（シェイク後に1回だけ発生）
	std::unique_ptr<ParticleManager> particles_;          // この個体専用のパーティクル
	float explodeTimer_ = 0.0f;                           // 爆発演出の経過時間
	static constexpr float kExplosionLifetime_ = 0.7f;    // 爆発を見せてから消えるまでの時間（秒）
	static constexpr const char* kExplosionPreset_ = "Explosion"; // Resources/Particle/Explosion.json
	static constexpr float kExplosionOffsetY_ = 1.0f;     // 爆発の発生高さ（体の中心あたり）
};
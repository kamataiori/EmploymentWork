#pragma once
#include "ObjectBase.h"
#include "ITarget.h"
#include <CollisionTypeIdDef.h>
#include "ParticleManager.h"
#include <memory>

class IDamagePopupSink;

//======================================================
// SwarmEnemy（第1波の雑魚：群れ突進）
//------------------------------------------------------
// ボス戦の MinionEnemy（1体ずつ空中スラム）とは別物。
// ボス不在で全員同時に自律行動する近接スウォーム：
//   追尾(Chase) → 一定距離でランジ(Lunge) → 硬直して離れる(Recover) → 追尾
// 当たると接触ダメージ（本体は Enemy グループなので Player 側の既存処理で被弾する）。
// ランジ後に一旦離れるので、接触は1回ずつ区切られる（多段ヒットにならない）。
//======================================================
class SwarmEnemy : public ObjectBase, public ITarget
{
public:
    SwarmEnemy(BaseScene* scene);
    ~SwarmEnemy() override = default;

    void Initialize() override {}

    // 出現位置を指定して初期化
    void InitializeSwarm(const Vector3& spawnPos);

    void Update() override;

    void BackGroundDraw() override {}
    void Draw() override;
    void ForeGroundDraw() override {}
    void ParticleDraw() override;
    void AnimationDraw() override {}

    void OnCollision() override {}
    void OnCollision(const CollisionInfo& info) override;

    bool IsDead() const { return isDead_; }

    //=== ITarget ===
    bool IsAlive() const override { return !isDead_; }
    Vector3 GetTargetCenter() const override { return transform.translate + Vector3{ 0.0f, kTargetOffsetY_, 0.0f }; }
    void ApplyDamage(int amount) override;

    void SetCamera(Camera* camera) override;

    // 追尾対象（プレイヤー）の Transform を渡す
    void SetPlayerTarget(const Transform* t) { playerTarget_ = t; }

    // ダメージ数値ポップアップの注入口（Scene 所有）
    void SetDamagePopupSink(IDamagePopupSink* sink) { damageSink_ = sink; }

private:
    // 挙動の3局面（switchを使わず if/else で分岐する）
    enum class Phase { Chase, Lunge, Recover };
    Phase phase_ = Phase::Chase;

    bool isDead_ = false;

    const Transform* playerTarget_ = nullptr;

    // 見た目・配置
    static constexpr const char* kModelName = "minion.obj";
    static constexpr float kModelScale     = 1.0f;
    static constexpr float kGroundOffsetY  = 1.0f;  // 足元を地面に合わせる持ち上げ量
    static constexpr float kTargetOffsetY_ = 1.0f;
    float groundY_ = 0.0f;                           // 立ち位置のY（地上維持の基準）

    // 追尾
    static constexpr float kChaseSpeed = 12.0f;
    static constexpr float kTurnLerp   = 0.25f;

    // ランジ（短い突進）
    static constexpr float kLungeRange    = 7.0f;   // この距離まで詰めたら突進開始
    static constexpr float kLungeSpeed    = 34.0f;
    static constexpr float kLungeDuration = 0.22f;
    Vector3 lungeDir_{ 0.0f, 0.0f, 1.0f };
    float   lungeTimer_ = 0.0f;

    // 硬直（突進後に少し離れて仕切り直す＝接触を1回ずつ区切る）
    static constexpr float kRecoverDuration = 0.6f;
    static constexpr float kRecoverBackSpeed = 6.0f; // プレイヤーと反対へ後退する速さ
    float recoverTimer_ = 0.0f;

    // コライダー
    static constexpr float kRadius = 0.8f;

    // HP（プレイヤー攻撃で減る。脆い）
    static constexpr int kMaxHP_       = 4;
    static constexpr int kDamagePerHit = 2;
    int hp_ = kMaxHP_;

    // パーティクル
    std::unique_ptr<ParticleManager> particles_;
    static constexpr const char* kHitSparkPreset_  = "HitSpark";
    static constexpr const char* kExplosionPreset_ = "Explosion";
    static constexpr float kEffectOffsetY_ = 1.0f;

    IDamagePopupSink* damageSink_ = nullptr;
};

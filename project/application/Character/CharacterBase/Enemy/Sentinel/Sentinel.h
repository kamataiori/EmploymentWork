#pragma once
#include "ObjectBase.h"
#include "ITarget.h"
#include <CollisionTypeIdDef.h>
#include "ParticleManager.h"
#include <memory>

class IDamagePopupSink;
class MeshBVH;

//======================================================
// Sentinel（前哨の敵）
//------------------------------------------------------
// ボス戦の前段。ステージの四隅に複数体（既定4体）配置され、
// これを全滅させるとボスが登場する。
//
// 当たり判定は2層に分ける：
//   1) BVHメッシュ（Stage型・Blocking・静的）
//        …プレイヤーがめり込まず押し戻される「壁」。ステージと同じ方式。
//   2) 被弾スフィア（kSentinel＝Defaultグループ・Trigger）
//        …プレイヤーの武器が当たるとダメージ。Defaultグループなので
//          触れてもプレイヤーはダメージを受けない。
//
// 現状は攻撃してこない骨組み（破壊のみ）。反撃（弾）は後の工程で足す。
//======================================================
class Sentinel : public ObjectBase, public ITarget
{
public:
    Sentinel(BaseScene* scene);
    ~Sentinel() override = default;

    void Initialize() override {}

    // 実際の初期化（配置位置とスケールを指定）。BVHは配置時に1回だけ焼き込む。
    void InitializeSentinel(const Vector3& placePos, float scale = kDefaultScale);

    //--- 登場演出（下から迫り上がる）---
    // 地中へ沈めて待機させる（登場前・被弾不可）
    void SetHiddenUnderground();
    // 迫り上がりを開始する
    void StartSpawn();
    // 登場が終わって地表に立ったか
    bool IsSpawnFinished() const { return spawnState_ == SpawnState::Active; }

    // 破壊可否ゲート（中央コアは Sentinel 全滅まで false にしておく）
    void SetHittable(bool v) { hittable_ = v; }
    bool IsHittable() const { return hittable_; }

    // 登場前で地中に隠れている間は描画・当たり判定を出さない
    bool IsHidden() const { return spawnState_ == SpawnState::Hidden; }

    void Update() override;

    void BackGroundDraw() override {}
    void Draw() override;
    void ForeGroundDraw() override {}
    void ParticleDraw() override;
    void AnimationDraw() override {}

    void OnCollision() override {}
    void OnCollision(const CollisionInfo& info) override;

    bool IsDead() const { return isDead_; }

    //=== ITarget（プレイヤーの攻撃対象としてのインターフェイス）===
    bool IsAlive() const override { return !isDead_; }
    Vector3 GetTargetCenter() const override { return transform.translate + Vector3{ 0.0f, kTargetOffsetY_, 0.0f }; }
    void ApplyDamage(int amount) override;

    void SetCamera(Camera* camera) override;

    // BVH（押し戻し）用コライダー。Scene がステージと同じ枠で登録する。
    MultiCollider* GetBvhCollider() const { return bvhCollider_.get(); }

    // ダメージ数値ポップアップの注入口（Scene 所有）
    void SetDamagePopupSink(IDamagePopupSink* sink) { damageSink_ = sink; }

private:
    // 見た目・配置
    static constexpr const char* kModelName = "EnemyCore.obj";
    static constexpr float kDefaultScale   = 2.0f;        // 既定スケール（四隅のSentinel）
    static constexpr float kGroundOffsetY  = 0.0f;        // 足元を地面に合わせる持ち上げ量
    static constexpr float kTargetOffsetY_ = 1.0f;        // 狙われる中心の高さ（胴体あたり）

    // 登場演出（下から迫り上がる）
    enum class SpawnState { Active, Hidden, Rising };
    SpawnState spawnState_ = SpawnState::Active;
    float surfaceY_ = 0.0f;                            // 立ち位置のY（迫り上がりの到達点）
    static constexpr float kSpawnDepth_     = 8.0f;    // 地表からどれだけ潜って始まるか
    static constexpr float kSpawnRiseSpeed_ = 10.0f;   // 迫り上がり速度

    // 破壊可否（false の間は殴ってもダメージが入らない）
    bool hittable_ = true;

    // 被弾スフィア
    static constexpr float kHitRadius = 2.0f;

    // HP
    static constexpr int kMaxHP_       = 6;
    static constexpr int kDamagePerHit = 2; // 武器1ヒットで減る量（6HPに対して3ヒットで撃破）
    int hp_ = kMaxHP_;

    bool isDead_ = false;

    // BVHメッシュ（押し戻し用・静的）
    std::shared_ptr<MeshBVH>        bvh_;
    std::unique_ptr<MultiCollider>  bvhCollider_;

    // 焼き込み：object3d_ のワールド行列でメッシュ三角形をワールド空間へ変換しBVHを構築する
    void BuildBvhCollider();

    // パーティクル（被弾火花・撃破爆発）
    std::unique_ptr<ParticleManager> particles_;
    static constexpr const char* kHitSparkPreset_  = "HitSpark";  // Resources/Particle/HitSpark.json
    static constexpr const char* kExplosionPreset_ = "Explosion"; // Resources/Particle/Explosion.json
    static constexpr float kEffectOffsetY_ = 1.0f;                // エフェクトの発生高さ

    // ダメージ数値ポップアップの注入口（Scene 所有・参照のみ）
    IDamagePopupSink* damageSink_ = nullptr;
};

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
    // 迫り上がりを開始する。地表に立ち切った時点で破壊可能になる。
    //   四隅 … SentinelRevealState（群れを全滅させた直後の演出）で呼ぶ
    //   コア … CoreBattleState（四隅を全滅させた後）で呼ぶ
    void StartSpawn();
    // 登場が終わって地表に立ったか
    bool IsSpawnFinished() const { return spawnState_ == SpawnState::Active; }

    // 破壊可否ゲート（中央コアは Sentinel 全滅まで false にしておく）
    void SetHittable(bool v) { hittable_ = v; }
    bool IsHittable() const { return hittable_; }

    //--- 弱点（コア）の位置 ---
    // 被弾スフィア・狙われる点・被弾火花・コアの光は、すべてこの一点に集まる。
    // 既定はモデル中心の少し上。中央コアのように図体が大きいと、弱点がメッシュに
    // 埋まってプレイヤーから見えず、BVH に阻まれて剣も届かなくなるので、
    // その場合は手前(-Z)へずらして面の外へ出す。
    // ※ここで動くのは弱点だけ。押し戻し用の BVH（モデルの当たり）は動かない。
    void SetCoreOffset(const Vector3& offset);
    // 弱点のワールド座標
    Vector3 GetWeakPointPos() const { return transform.translate + coreOffset_; }

    // 中央コア用の「その場で光り続けるコア」を出す（四隅は呼ばないので付かない）
    void EnableCoreGlow();

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
    // 狙う点＝弱点。突進乱舞はここへ飛び込む
    Vector3 GetTargetCenter() const override { return GetWeakPointPos(); }
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

    // 弱点のモデル原点からのオフセット。既定は従来どおり中心の少し上。
    // 中央コアだけ Scene から手前へずらす（SetCoreOffset）。
    Vector3 coreOffset_{ 0.0f, kTargetOffsetY_, 0.0f };

    // 登場演出（下から迫り上がる）
    enum class SpawnState { Active, Hidden, Rising };
    SpawnState spawnState_ = SpawnState::Active;
    float surfaceY_ = 0.0f;                            // 立ち位置のY（迫り上がりの到達点）

    // 潜る深さはスケールに比例させる。EnemyCore.obj はスケール1で高さ約3.5あるので、
    // 固定値だと大きい個体（中央コア＝スケール4）が頭を出したまま隠れない。
    static constexpr float kSpawnDepthPerScale_ = 4.0f;
    // 迫り上がりにかける時間。深さが変わっても登場の尺は揃える
    static constexpr float kSpawnRiseDuration_  = 0.8f;
    float spawnDepth_     = 0.0f;                      // この個体が潜る深さ（スケールで決まる）
    float spawnRiseSpeed_ = 0.0f;                      // この個体の迫り上がり速度

    // 破壊可否（false の間は殴ってもダメージが入らない）。
    // 地中待機中・迫り上がり中は false で、地表に立ち切った時点で true になる。
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

    // 紫のオーラ（足元から湧き上がる持続エミッタ）。
    // 実体は particles_ が持つので、ここは参照するだけのポインタ。
    static constexpr const char* kAuraPreset_ = "SentinelAura";   // Resources/Particle/SentinelAura.json
    static constexpr float kAuraOffsetY_ = 0.0f;                  // 湧き上がりの発生高さ（足元）
    ParticleEmitterInstance* auraEmitter_ = nullptr;
    bool auraPlaying_ = false;

    // コアの光（弱点の位置で光り続ける持続エミッタ）。
    // EnableCoreGlow() を呼んだ個体だけが持つ（＝中央コアのみ）。
    static constexpr const char* kCoreGlowPreset_ = "CoreGlow";   // Resources/Particle/CoreGlow.json
    ParticleEmitterInstance* coreGlowEmitter_ = nullptr;
    bool coreGlowPlaying_ = false;

    // 持続エミッタの追従と再生/停止。地中待機中と破壊後は出さない
    void UpdateEmitters();

    // ダメージ数値ポップアップの注入口（Scene 所有・参照のみ）
    IDamagePopupSink* damageSink_ = nullptr;
};

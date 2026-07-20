#pragma once
#include "ObjectBase.h"
#include <CollisionTypeIdDef.h>

//======================================================
// EnemyCoreBullet
//------------------------------------------------------
// コアが撃つ弾。撃った瞬間のプレイヤー位置へ向かって直進するだけ（誘導しない）。
// ボスの EnemySplitBullet と違い上昇も分裂もしないので、動きは等速直線のみ。
//
// 当たり判定は EnemyBullet（Enemyグループ）なので、プレイヤーに触れれば
// Player 側の既存処理でダメージが入る。
//======================================================
class EnemyCoreBullet : public ObjectBase
{
public:
    EnemyCoreBullet(BaseScene* scene) : ObjectBase(scene) {}
    ~EnemyCoreBullet() override = default;

    void Initialize() override {}

    // start から targetPos の方向へ発射する
    void Fire(const Vector3& start, const Vector3& targetPos);

    void Update() override;

    void BackGroundDraw() override {}
    void Draw() override;
    void ForeGroundDraw() override {}
    void ParticleDraw() override;
    void AnimationDraw() override {}

    void OnCollision() override { isDead_ = true; }
    void OnCollision(const CollisionInfo& info) override;

    bool IsDead() const { return isDead_; }

    void SetCamera(Camera* camera) override;

private:
    static constexpr float kRadius       = 0.6f;   // 当たり判定の半径
    static constexpr float kSpeed        = 28.0f;  // 直進速度
    static constexpr float kLifeSeconds  = 6.0f;   // 誰にも当たらなかった弾の寿命
    static constexpr float kEmitInterval = 0.03f;  // 尾を引くパーティクルの発生間隔

    static constexpr const char* kTrailSystemName = "EnemyBullet";

    Vector3 velocity_{ 0.0f, 0.0f, 0.0f };
    float   life_ = kLifeSeconds;
    bool    isDead_ = false;

    std::unique_ptr<ParticleManager> particles_;
    Transform particleTransform_{};
    float emitTimer_ = 0.0f;
};

#pragma once
#include "ObjectBase.h"
#include <CollisionTypeIdDef.h>

// 「打ち上げ→落下」弾（今はデバッグ球のみ表示）
class EnemyDropBullet : public ObjectBase
{
public:
    EnemyDropBullet(BaseScene* scene) : ObjectBase(scene) {}
    ~EnemyDropBullet() override = default;

    void Initialize() override;
    // shootPos : 発射位置（敵の位置など）
    // targetPos: 落下させたい地点（プレイヤー位置をロック）
    void Initialize(const Vector3& shootPos, const Vector3& targetPos);

    void Update() override;
    void BackGroundDraw() override {}
    void Draw() override;
    void ForeGroundDraw() override {}
    void ParticleDraw() override {}
    void AnimationDraw() override {}

    void OnCollision() override;
    void OnCollision(const CollisionInfo& info) override;

    bool IsDead() const { return isDead_; }

private:
    enum class Phase { Rise, Fall };
    Phase phase_ = Phase::Rise;

    Vector3 startPos_{};
    Vector3 targetPos_{};

    float radius_ = 0.8f;

    float riseSpeed_ = 18.0f;      // 上昇速度
    float apexHeight_ = 14.0f;     // ここまで上がったら落下へ（startPos.y + apexHeight_）

    float fallSpeed_ = 22.0f;      // 落下速度（Y）
    float homingLerp_ = 10.0f;     // 落下中のXZ補間（大きいほど狙う）

    float life_ = 6.0f;
    float timer_ = 0.0f;

    bool isDead_ = false;
};
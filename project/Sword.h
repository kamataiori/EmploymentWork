#pragma once
#include "ObjectBase.h"
#include <string>

class Sword : public ObjectBase
{
public:
    Sword(BaseScene* scene) : ObjectBase(scene) {}

    void Initialize() override;
    void Update() override;

    void BackGroundDraw() override;
    void Draw() override;              // コライダー可視化など
    void ForeGroundDraw() override;
    void ParticleDraw() override;
    void AnimationDraw() override;     // ここで剣モデル描画
    void OnCollision() override;

    // プレイヤーのボーンに装着
    void AttachTo(Object3d* ownerObj, const std::string& jointName);

    // 剣の「手からのオフセット」
    void SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s);

    // 攻撃中だけ当たり判定ONにしたい用
    void SetHitEnabled(bool enabled) { hitEnabled_ = enabled; }

private:
    Object3d* ownerObj_ = nullptr;
    std::string ownerJoint_;

    // 当たり判定（簡易：球1個）
    bool hitEnabled_ = false;
    float hitRadius_ = 0.6f;
    Vector3 hitOffset_ = { 0.0f, 0.0f, 1.2f }; // 剣先方向へずらす（調整用）
    bool isHit_ = false;

    // 握り位置（ボーンに対するローカルオフセットのデフォルト）
    Vector3 defaultOffsetT_ = { -0.44f, -0.73f, -0.48f };
    Vector3 defaultOffsetR_ = { -0.56f, 4.95f, 0.0f };

    Vector3 defaultOffsetS_ = { 1.0f, 1.0f, 1.0f };
};

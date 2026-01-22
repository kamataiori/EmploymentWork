#pragma once
#include "ObjectBase.h"

class PlayerBullet : public ObjectBase
{
public:
    PlayerBullet(BaseScene* scene) : ObjectBase(baseScene_) {};
    
    // ====== ObjectBase override ======
    void Initialize() override;
    void Update() override;
    /// <summary>
    /// 背景スプライト処理
    /// </summary>
    void BackGroundDraw() override;
    void Draw() override;
    /// <summary>
    /// 前景スプライト処理
    /// </summary>
    void ForeGroundDraw() override;
    void AnimationDraw() override;
    void ParticleDraw() override;
    void OnCollision() override;

    // 弾の発射初期化
    void Fire(const Vector3& start, const Vector3& dir, float speed = 0.6f, float lifeSec = 3.0f);

    bool IsDead() const { return life_ <= 0.0f; }

private:
    Vector3 velocity_{};
    float life_ = 0.0f;

    // コライダー情報
    float colliderRadius_ = 0.3f;

    // 衝突フラグ（ImGui確認用）
    bool isCollided_ = false;
};


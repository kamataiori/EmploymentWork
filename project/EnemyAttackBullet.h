#pragma once
#include "CharacterBase.h"
#include "SphereCollider.h"

class EnemyAttackBullet : public CharacterBase, public SphereCollider {
public:
    EnemyAttackBullet(BaseScene* scene);
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollision() override;

    void SetVelocity(const Vector3& v) { velocity_ = v; }
    bool IsDead() const { return isDead_; }

    void SetTranslate(const Vector3& pos) { transform.translate = pos; }

    void SetCamera(Camera* camera) {
        camera_ = camera;
        object3d_->SetCamera(camera);
    }

private:
    Vector3 velocity_ = {};
    float lifetime_ = 0.0f;
    float maxLifetime_ = 3.0f;
    bool isDead_ = false;
};

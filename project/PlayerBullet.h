#pragma once
#include "Object3d.h"
#include "Transform.h"

class PlayerBullet {
public:
    PlayerBullet(const Vector3& pos, const Vector3& dir, BaseScene* scene, Camera* camera);

    void Update();
    void Draw();

    bool IsDead() const { return isDead_; }

private:
    std::unique_ptr<Object3d> object_;
    Transform transform_;
    Vector3 velocity_;
    float speed_ = 4.0f;
    bool isDead_ = false;
    float lifeTime_ = 2.0f; // 秒数
};

#include "EnemyAreaAttack.h"

EnemyAreaAttack::EnemyAreaAttack(BaseScene* scene)
    : CharacterBase(scene), SphereCollider(sphere)
{
    SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyAreaAttack));
}

void EnemyAreaAttack::Initialize() {
    // SphereColliderの半径設定（炎の範囲などに応じて）
    SetRadius(5.0f);
    SetPosition(transform.translate);
}

void EnemyAreaAttack::Update() {
    lifetime_ += 1.0f / 60.0f;
    if (lifetime_ >= maxLifetime_) {
        isDead_ = true; // 寿命で消滅
    }

    SetPosition(transform.translate);
    sphere.color = static_cast<int>(Color::WHITE);
}

void EnemyAreaAttack::Draw() {
    SphereCollider::Draw();
}

void EnemyAreaAttack::OnCollision()
{
    sphere.color = static_cast<int>(Color::BLUE);
}

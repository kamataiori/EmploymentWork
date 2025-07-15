#include "EnemyAttackBullet.h"

EnemyAttackBullet::EnemyAttackBullet(BaseScene* scene)
    : CharacterBase(scene), SphereCollider(sphere) {
    SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyBullet));
}

void EnemyAttackBullet::Initialize() {
    object3d_->Initialize();
    ModelManager::GetInstance()->LoadModel("bullet.obj");
    object3d_->SetModel("bullet.obj");

    transform.scale = { 0.8f, 0.8f, 0.8f };
    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);

    SetCollider(this);
    SetPosition(transform.translate);
    SetRadius(1.0f);
}

void EnemyAttackBullet::Update() {
    transform.translate += velocity_;

    lifetime_ += 1.0f / 60.0f;
    if (lifetime_ >= maxLifetime_) {
        isDead_ = true;
    }

    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);
    object3d_->Update();

    SetPosition(transform.translate);
    sphere.color = static_cast<int>(Color::WHITE);
}

void EnemyAttackBullet::Draw() {
    object3d_->Draw();
    SphereCollider::Draw();
}

void EnemyAttackBullet::SkinningDraw()
{
}

void EnemyAttackBullet::ParticleDraw()
{
}

void EnemyAttackBullet::OnCollision()
{
    sphere.color = static_cast<int>(Color::PURPLE);
    isDead_ = true;
}

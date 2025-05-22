#include "PlayerBullet.h"

PlayerBullet::PlayerBullet(const Vector3& pos, const Vector3& dir, BaseScene* scene, Camera* camera) {
    ModelManager::GetInstance()->LoadModel("multiMesh.obj");

    object_ = std::make_unique<Object3d>(scene);
    object_->Initialize();
    object_->SetModel("multiMesh.obj");
    object_->SetCamera(camera);

    transform_.translate = pos;
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = {};

    velocity_ = Normalize(dir) * speed_;
}

void PlayerBullet::Update() {
    lifeTime_ -= 1.0f / 60.0f;
    if (lifeTime_ <= 0.0f) {
        isDead_ = true;
        return;
    }

    transform_.translate += velocity_;

    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);
    object_->Update();

#ifdef _DEBUG
    ImGui::Begin("PlayerBullet");

    ImGui::Text("Position: (%.2f, %.2f, %.2f)",
        transform_.translate.x,
        transform_.translate.y,
        transform_.translate.z);

    ImGui::Text("Velocity: (%.2f, %.2f, %.2f)",
        velocity_.x,
        velocity_.y,
        velocity_.z);

    ImGui::Text("Lifetime: %.2f", lifeTime_);

    ImGui::End();
#endif
}

void PlayerBullet::Draw() {
    if (!isDead_) {
        object_->Draw();
    }
}

#include "PlayerBullet.h"
#include <algorithm>

void PlayerBullet::Initialize()
{
    // モデル読み込み
    ModelManager::GetInstance()->LoadModel("playerBullet.obj");
    object3d_->Initialize();
    object3d_->SetModel("playerBullet.obj");

    // Transform初期化
    transform.scale = { 1.0f, 1.0f, 1.0f };

    // ==== コライダー初期化 ====
    Sphere sphere{};
    sphere.center = transform.translate;
    sphere.radius = colliderRadius_;

    Shape shape{};
    shape.kind = ShapeKind::Sphere;
    shape.sphere = sphere;

    *multiCollider_ = MultiCollider(shape);
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerWeapon));

}

void PlayerBullet::Update()
{
    // 当たったらフラグを立てる
    multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

    if (IsDead()) return;

    // 移動
    transform.translate += velocity_;

    // 寿命
    life_ -= 1.0f / 60.0f;
    if (life_ < 0.0f) life_ = 0.0f;

    // Object3d更新
    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);
    object3d_->Update();

    // コライダー位置も更新
    Sphere& s = multiCollider_->MutableSphere(0);
    s.center = transform.translate;
    s.radius = colliderRadius_;

#ifdef DEBUG

    // ==== ImGuiデバッグ ====
    ImGui::Begin("PlayerBullet");
    ImGui::Text("Pos: (%.2f, %.2f, %.2f)", transform.translate.x, transform.translate.y, transform.translate.z);
    ImGui::Text("Life: %.2f", life_);
    ImGui::Text("Collided: %s", isCollided_ ? "TRUE" : "FALSE");
    ImGui::End();

#endif // DEBUG

    // 当たり後リセット
    isCollided_ = false;
}

void PlayerBullet::BackGroundDraw()
{
}

void PlayerBullet::Draw()
{
    if (IsDead()) return;

    object3d_->Draw();
    multiCollider_->Draw();
}

void PlayerBullet::ForeGroundDraw()
{
}

void PlayerBullet::AnimationDraw()
{
}

void PlayerBullet::ParticleDraw()
{
}

void PlayerBullet::OnCollision()
{
    // 当たった瞬間に消す or フラグ立てる
    isCollided_ = true;
    life_ = 0.0f;
}

void PlayerBullet::Fire(const Vector3& start, const Vector3& dir, float speed, float lifeSec)
{
    transform.translate = start;
    velocity_ = dir * speed;
    life_ = lifeSec;
}

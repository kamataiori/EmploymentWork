#include "Enemy.h"
#include <CollisionTypeIdDef.h>

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");

	object3d_->SetModel("uvChecker.gltf");

	// モデルにSRTを設定
	object3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	object3d_->SetRotate({ 0.0f, 3.14f, 0.0f });
	object3d_->SetTranslate({ 2.0f, 0.0f, 0.0f });

	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	//SetRotation(object3d_->GetRotate());
	//SetScale(object3d_->GetScale());
	sphere.radius = 1.5f;

	//SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
}

void Enemy::Update()
{
	ImGui::Begin("Enemy Transform");

	// ✅ Translate (位置)
	Vector3 position = object3d_->GetTranslate();
	if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
		object3d_->SetTranslate(position);
	}

	// ✅ Rotate (回転)
	Vector3 rotation = object3d_->GetRotate();
	if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
		object3d_->SetRotate(rotation);
	}

	// ✅ Scale (スケール)
	Vector3 scale = object3d_->GetScale();
	if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
		object3d_->SetScale(scale);
	}

	ImGui::End();

	object3d_->Update();
	SetPosition(object3d_->GetTranslate());
	//SetScale(object3d_->GetScale());
	sphere.color = static_cast<int>(Color::WHITE);
}

void Enemy::Draw()
{
	object3d_->Draw();
	// SphereCollider の描画
	SphereCollider::Draw();
}

void Enemy::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);
}

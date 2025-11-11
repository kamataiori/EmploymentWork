#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include "Player.h"

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	/*ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");*/
	ModelManager::GetInstance()->LoadModel("matest.obj");
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");
	ModelManager::GetInstance()->LoadModel("Sam.gltf");

	object3d_->SetModel("Skeleton.gltf");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 3.0f, 3.0f, 3.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->SetAnimation(animation_.Idle);

	// ---- Sphereコライダー設定 ----
	colliderTranslate_ = transform.translate + colliderOffset_; // モデル原点からオフセット
	Sphere enemySp{};
	enemySp.center = colliderTranslate_;
	enemySp.radius = sphereRadius_;

	Shape first{};
	first.kind = ShapeKind::Sphere;
	first.sphere = enemySp;

	*multiCollider_ = MultiCollider(first);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

}

void Enemy::Update()
{

	//ImGui::Begin("Enemy Transform");

	//// Translate (位置)
	//Vector3 position = object3d_->GetTranslate();
	//if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
	//	object3d_->SetTranslate(position);
	//}

	//// Rotate (回転)
	//Vector3 rotation = object3d_->GetRotate();
	//if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
	//	object3d_->SetRotate(rotation);
	//}

	//// Scale (スケール)
	//Vector3 scale = object3d_->GetScale();
	//if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
	//	object3d_->SetScale(scale);
	//}

	//ImGui::End();

	ImGui::Begin("Enemy");
	ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
	ImGui::DragFloat3("Collider Offset", &colliderOffset_.x, 0.01f);
	ImGui::DragFloat("Sphere Radius", &sphereRadius_, 0.01f, 0.0f, 50.0f);
	ImGui::End();

	// 当たり判定中心を更新
	colliderTranslate_ = transform.translate + colliderOffset_;

	// コライダー更新
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;

	// ------------------------
	// オブジェクト更新処理
	// ------------------------
	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

}

void Enemy::Draw()
{
	// コライダーの描画
	multiCollider_->Draw();
}

void Enemy::DrawModel()
{
	object3d_->Draw();
}

void Enemy::SkinningDraw()
{
}

void Enemy::ParticleDraw()
{
}

void Enemy::OnCollision()
{
}

void Enemy::SetAnimationIfChanged(const std::string& name)
{
	if (currentAnimationName_ != name) {
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}
}

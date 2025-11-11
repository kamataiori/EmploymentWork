#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include "Player.h"

// Yaw(=Y回転)から OBB の3軸を作る簡易ヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	// 右(X), 上(Y), 前(Z)
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f,  0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

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
	//colliderTranslate_ = transform.translate + colliderOffset_; // モデル原点からオフセット
	//Sphere enemySp{};
	//enemySp.center = colliderTranslate_;
	//enemySp.radius = sphereRadius_;

	//Shape first{};
	//first.kind = ShapeKind::Sphere;
	//first.sphere = enemySp;

	//*multiCollider_ = MultiCollider(first);
	//multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	//multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

	// ---- OBB コライダー初期化 ----
	colliderCenter_ = transform.translate + colliderOffset_;

	OBB obb{};
	obb.center = colliderCenter_;
	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];
	obb.size = obbSize_;  // 半径

	Shape first{};
	first.kind = ShapeKind::OBB;
	first.obb = obb;

	*multiCollider_ = MultiCollider(first);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	// ヒットを Enemy::OnCollision に橋渡し
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
	//ImGui::DragFloat("Sphere Radius", &sphereRadius_, 0.01f, 0.0f, 50.0f);
	ImGui::DragFloat3("OBB Size (half)", &obbSize_.x, 0.01f, 0.0f, 50.0f);
	ImGui::DragFloat("Yaw (rad)", &transform.rotate.y, 0.01f);
	ImGui::End();

	// 当たり判定中心を更新
	//colliderTranslate_ = transform.translate + colliderOffset_;
	colliderCenter_ = transform.translate + colliderOffset_;

	// コライダー更新
	/*Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;*/

	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	// スケールを当たりにも反映したい場合はここで掛ける
	obb.size = { obbSize_.x /** transform.scale.x*/,
				 obbSize_.y /** transform.scale.y*/,
				 obbSize_.z /** transform.scale.z*/ };

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

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
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	object3d_->SetAnimation(animation_.Idle);

}

void Enemy::Update()
{
	//// ImGuiデバッグ表示
	//ImGui::Begin("Enemy Debug");

	//if (currentState_) {
	//	// 現在のステート名を表示
	//	ImGui::Text("Current State: %s", currentState_->GetName());
	//}
	//else {
	//	ImGui::Text("Current State: None");
	//}

	//ImGui::End();

	// プレイヤーの方向を向く
	//if (player_) {
	//	Vector3 toPlayer = player_->GetTransform().translate - this->GetTransform().translate;

	//	// Y軸方向だけで角度を計算（上下方向は無視）
	//	float angleY = std::atan2(toPlayer.x, toPlayer.z);

	//	transform.rotate.y = angleY;
	//}


	


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

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

}

void Enemy::Draw()
{



	// SphereCollider の描画
	//SphereCollider::Draw();
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

Vector3 Enemy::GetPlayerPos() const
{
	/*if (player_) {
		return player_->GetTransform().translate;
	}*/
	return { 0, 0, 0 }; // 参照が無ければ原点
}

void Enemy::SetAnimationIfChanged(const std::string& name)
{
	if (currentAnimationName_ != name) {
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}
}

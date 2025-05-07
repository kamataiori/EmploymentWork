#include "Player.h"
#include <CollisionTypeIdDef.h>

void Player::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");

	object3d_->SetModel("uvChecker.gltf");

	transform_.translate = { -2.0f, 0.0f, 0.0f };
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 3.14f, 0.0f };

	// モデルにSRTを設定
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);

	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	sphere.radius = 2.0f;
	//SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));
}

void Player::Update()
{
	// velocityをリセット
	velocity_ = {};

	if (Input::GetInstance()->PushKey(DIK_W))
	{
		velocity_.z = 0.2f;
	}
	if (Input::GetInstance()->PushKey(DIK_A))
	{
		velocity_.x = -0.2f;
	}
	if (Input::GetInstance()->PushKey(DIK_S))
	{
		velocity_.z = -0.2f;
	}
	if (Input::GetInstance()->PushKey(DIK_D))
	{
		velocity_.x = 0.2f;
	}

	transform_.translate += velocity_;

	Debug();

	// モデルにSRTを設定
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
	SetPosition(object3d_->GetTranslate());
}

void Player::Draw()
{
	object3d_->Draw();
	// SphereCollider の描画
	SphereCollider::Draw();
}

void Player::Debug()
{
#ifdef _DEBUG

	ImGui::Begin("Player Transform");

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

#endif // DEBUG
}

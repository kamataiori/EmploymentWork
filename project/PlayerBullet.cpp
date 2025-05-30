#include "PlayerBullet.h"

PlayerBullet::PlayerBullet(BaseScene* baseScene)
	: CharacterBase(baseScene), SphereCollider(sphere) {
}

void PlayerBullet::Initialize()
{
	object3d_->Initialize();
	ModelManager::GetInstance()->LoadModel("bullet.obj");

	object3d_->SetModel("bullet.obj");

	// Transform 設定（SetPosition で先に設定していれば反映される）
	transform.scale = { 0.8f, 0.8f, 0.8f };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	// コライダー設定
	SetCollider(this);
	SetPosition(transform.translate);

	sphere.radius = 1.0f;
}

void PlayerBullet::Update()
{
	// 弾を前進させる
	transform.translate += velocity_;

	// transform を object3d_ に反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// コライダー位置更新
	SetPosition(transform.translate);

	// 寿命タイマー更新
	lifeTimer_ += 1.0f / 60.0f; // 仮に60FPS前提でフレームタイミングで更新
}

void PlayerBullet::Draw()
{
	object3d_->Draw();
	SphereCollider::Draw(); // デバッグ用描画
}

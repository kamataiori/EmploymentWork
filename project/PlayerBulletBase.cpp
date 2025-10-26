#include "PlayerBulletBase.h"
#include "BaseScene.h"
#include <cassert>

void PlayerBulletBase::Initialize(BaseScene* scene)
{
	scene_ = scene;


	// 見た目（任意）
	if (const char* model = GetModelName()) {
		obj_ = std::make_unique<Object3d>(scene_);
		obj_->Initialize();
		obj_->SetModel(model);
	}

	// コライダの属性（プレイヤ弾）
	SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerBullet));
	sphere_.radius = 0.2f;
}

void PlayerBulletBase::Shoot(const Vector3& pos, const Vector3& dir, float speed, float lifeSec)
{
	pos_ = pos;
	dir_ = (Length(dir) > 0.0f) ? Normalize(dir) : Vector3{ 0,0,1 };
	speed_ = speed;
	lifeSec_ = lifeSec;
	alive_ = true;


	SetPosition(pos_);
	if (obj_) {
		obj_->SetTranslate(pos_);
		obj_->Update();
	}
}

void PlayerBulletBase::Update()
{
	if (!alive_) return;


	// 移動
	pos_ += dir_ * speed_;


	// 寿命
	lifeSec_ -= kDt;
	if (lifeSec_ <= 0.0f) {
		alive_ = false;
	}


	// 反映
	SetPosition(pos_);
	if (obj_) {
		obj_->SetTranslate(pos_);
		obj_->Update();
	}


	// 追加処理（軌跡・加速など）
	OnUpdate();
}

void PlayerBulletBase::Draw()
{
	if (!alive_) return;


	// デバッグ: 当たり表示
	SphereCollider::Draw();


	if (obj_) obj_->Draw();
}

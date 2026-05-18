#include "MinionEnemy.h"
#include "engine/TimeManager.h"
#include <cmath>

static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

static Vector3 NormalizeSafe(const Vector3& v)
{
	float len = Length(v);
	if (len < 1e-6f) return { 0,0,1 };
	return v / len;
}

MinionEnemy::MinionEnemy(BaseScene* scene)
	: ObjectBase(scene)
{
}

void MinionEnemy::InitializeMinion(const Vector3& spawnPos,const Transform* targetTransform)
{
	ModelManager::GetInstance()->LoadModel("minion.obj");

	object3d_->Initialize();
	object3d_->SetModel("minion.obj");

	targetTransform_ = targetTransform;

	// 地面の位置を記憶
	surfacePos_ = spawnPos;

	// 地面の下からスタート
	transform.translate = spawnPos;
	transform.translate.y -= spawnDepth_;
	transform.rotate = { 0,0,0 };
	transform.scale = { 1,1,1 };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	phase_ = Phase::Spawn;
	isDead_ = false;
	hp_ = kMaxHP_;
	lifeTimer_ = 0.0f;

	// コライダー（Sphere）
	multiCollider_->Clear();

	Sphere sp{};
	sp.center = transform.translate;
	sp.radius = radius_;
	multiCollider_->AddSphere(sp);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });
}

void MinionEnemy::Update()
{
	if (isDead_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// 寿命チェック（Spawn中もカウントする）
	lifeTimer_ += dt;
	if (lifeTimer_ >= kMaxLifeTime_) {
		isDead_ = true;
		return;
	}

	switch (phase_)
	{
	case Phase::Spawn:
	{
		// 地面の下から上昇
		transform.translate.y += riseSpeed_ * dt;

		// 地面の高さに到達したら追跡開始
		if (transform.translate.y >= surfacePos_.y) {
			transform.translate.y = surfacePos_.y;
			phase_ = Phase::Chase;
		}
		break;
	}

	case Phase::Chase:
	{
		// プレイヤーに向かって移動
		if (targetTransform_) {
			Vector3 toTarget = targetTransform_->translate - transform.translate;
			toTarget.y = 0.0f;
			float dist = Length(toTarget);

			if (dist > 0.5f) {
				Vector3 dir = NormalizeSafe(toTarget);

				// 向き補間
				float desiredYaw = std::atan2(dir.x, dir.z);
				transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_);

				// 移動
				transform.translate.x += dir.x * moveSpeed_ * dt;
				transform.translate.z += dir.z * moveSpeed_ * dt;
			}
		}
		break;
	}
	}

	// コライダー更新
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = transform.translate;
	sp.radius = radius_;

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();
}

void MinionEnemy::Draw()
{
	multiCollider_->Draw();

	object3d_->Draw();
}

void MinionEnemy::OnCollision(const CollisionInfo& info)
{
	auto other = static_cast<CollisionTypeIdDef>(info.otherType);

	if (other == CollisionTypeIdDef::kPlayerWeapon ||
		other == CollisionTypeIdDef::kPlayerAttack ||
		other == CollisionTypeIdDef::PlayerBullet)
	{
		hp_ -= 1;
		if (hp_ <= 0) {
			isDead_ = true;
		}
	}
}

void MinionEnemy::SetCamera(Camera* camera)
{
	ObjectBase::SetCamera(camera);
}
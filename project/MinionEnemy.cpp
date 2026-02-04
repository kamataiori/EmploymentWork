#include "MinionEnemy.h"
#include <cmath>

float MinionEnemy::WrapDeltaRad(float a)
{
	while (a > 3.14159265f)  a -= 6.2831853f;
	while (a < -3.14159265f) a += 6.2831853f;
	return a;
}

float MinionEnemy::LerpAngleRad(float from, float to, float t)
{
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

MinionEnemy::MinionEnemy(BaseScene* baseScene)
	: ObjectBase(baseScene)
{
}

void MinionEnemy::Initialize()
{
	ModelManager::GetInstance()->LoadModel("matest.obj");

	object3d_->Initialize();
	object3d_->SetModel("matest.obj");

	transform.scale = { 1,1,1 };
	transform.rotate = { 0,0,0 };
	transform.translate = { 0, riseStartY_, 0 };
}

void MinionEnemy::SetCamera(Camera* camera)
{
	// Enemy と同じように ObjectBase 側に任せる
	ObjectBase::SetCamera(camera);
}

void MinionEnemy::SetTargetTransform(const Transform* target)
{
	target_ = target;
}

void MinionEnemy::SetSpawnPositionXZ(const Vector3& posXZ)
{
	transform.translate = posXZ;
	transform.translate.y = riseStartY_;
	state_ = State::Rising;
	readyTimer_ = 0.0f;
}

void MinionEnemy::SetRiseParams(float startY, float endY, float speed)
{
	riseStartY_ = startY;
	riseEndY_ = endY;
	riseSpeed_ = speed;
}

void MinionEnemy::SetReadyDuration(float sec)
{
	readyDuration_ = sec;
}

void MinionEnemy::SetChaseParams(float speed, float turnLerp)
{
	chaseSpeed_ = speed;
	turnLerp_ = turnLerp;
}

void MinionEnemy::Update()
{
	const float dt = TimeManager::GetInstance()->GetDeltaTime();

	switch (state_)
	{
	case State::Rising:  UpdateRising(dt);  break;
	case State::Ready:   UpdateReady(dt);   break;
	case State::Chasing: UpdateChasing(dt); break;
	}

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();
}

void MinionEnemy::UpdateRising(float dt)
{
	transform.translate.y += riseSpeed_ * dt;

	if (transform.translate.y >= riseEndY_) {
		transform.translate.y = riseEndY_;
		state_ = State::Ready;
		readyTimer_ = 0.0f;
	}
}

void MinionEnemy::UpdateReady(float dt)
{
	readyTimer_ += dt;
	if (readyTimer_ >= readyDuration_) {
		state_ = State::Chasing;
	}
}

void MinionEnemy::UpdateChasing(float dt)
{
	if (!target_) return;

	Vector3 to = target_->translate - transform.translate;
	to.y = 0.0f;

	const float len = std::sqrt(to.x * to.x + to.z * to.z);
	if (len < 1e-6f) return;

	Vector3 dir = { to.x / len, 0.0f, to.z / len };

	const float yaw = std::atan2(dir.x, dir.z);
	transform.rotate.y = LerpAngleRad(transform.rotate.y, yaw, turnLerp_);

	transform.translate.x += dir.x * chaseSpeed_ * dt;
	transform.translate.z += dir.z * chaseSpeed_ * dt;
}

void MinionEnemy::Draw()
{
	object3d_->Draw();
}

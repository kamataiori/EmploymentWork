#include "Sword.h"
#include <CollisionTypeIdDef.h>
#include <cmath>

// Yaw(=Y回転)から OBB の3軸を作る
static void BuildYawAxes(float yaw, Vector3 outAxes[3])
{
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);

	// 右(X), 上(Y), 前(Z)
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f, 0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

// ローカルオフセットを yaw 回転してワールドへ
static Vector3 RotateOffsetByYaw(const Vector3& localOffset, float yaw)
{
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);

	return {
		localOffset.x * c + localOffset.z * s,
		localOffset.y,
	   -localOffset.x * s + localOffset.z * c
	};
}

void Sword::Initialize()
{
	object3d_->Initialize();

	ModelManager::GetInstance()->LoadModel("sword.obj");
	object3d_->SetModel("sword.obj");

	transform.translate = { 0.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	// ---- OBBコライダー初期化 ----
	OBB obb{};
	obb.center = { 0.0f, 0.0f, 0.0f };
	obb.orientations[0] = { 1.0f, 0.0f, 0.0f };
	obb.orientations[1] = { 0.0f, 1.0f, 0.0f };
	obb.orientations[2] = { 0.0f, 0.0f, 1.0f };
	obb.size = { 0.0f, 0.0f, 0.0f }; // 最初は無効

	Shape shape{};
	shape.kind = ShapeKind::OBB;
	shape.obb = obb;

	*multiCollider_ = MultiCollider(shape);

	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerWeapon));
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });
}

void Sword::AttachTo(Object3d* ownerObj, const std::string& jointName)
{
	ownerObj_ = ownerObj;
	ownerJoint_ = jointName;

	object3d_->SetParentJoint(ownerObj_, ownerJoint_);

	transform.translate = defaultOffsetT_;
	transform.rotate = defaultOffsetR_;
	transform.scale = defaultOffsetS_;
}

void Sword::SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s)
{
	transform.translate = t;
	transform.rotate = r;
	transform.scale = s;
}

void Sword::Update()
{
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// ---- OBB更新 ----
	OBB& obb = multiCollider_->MutableOBB(0);

	const Matrix4x4& world = object3d_->GetWorldMatrix();

	// 行から軸を取り出す
	Vector3 axisX = Normalize({ world.m[0][0], world.m[0][1], world.m[0][2] });
	Vector3 axisY = Normalize({ world.m[1][0], world.m[1][1], world.m[1][2] });
	Vector3 axisZ = Normalize({ world.m[2][0], world.m[2][1], world.m[2][2] });

	Vector3 worldPos = world.Translation();

	obb.center = worldPos;

	obb.orientations[0] = axisX;
	obb.orientations[1] = axisY;
	obb.orientations[2] = axisZ;

	obb.size = hitEnabled_ ? hitHalfSize_ : Vector3{ 0.0f, 0.0f, 0.0f };

	isHit_ = false;
}

void Sword::BackGroundDraw()
{
}

void Sword::Draw()
{
	multiCollider_->Draw();
	object3d_->Draw();
}

void Sword::ForeGroundDraw()
{
}

void Sword::ParticleDraw()
{
}

void Sword::AnimationDraw()
{
}

void Sword::OnCollision()
{
	isHit_ = true;
}
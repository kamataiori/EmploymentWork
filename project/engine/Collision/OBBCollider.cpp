#include "OBBCollider.h"
#include "CollisionFunctions.h"
#include "SphereCollider.h"
#include "AABBCollider.h"
#include "CapsuleCollider.h"

OBBCollider::OBBCollider(const OBB& box) : obb(box) {
    obb.center = { -1.9f, -0.3f, 0.0f };
    obb.orientations[0] = { 1.0f, 0.0f, 0.0f }; // X軸
    obb.orientations[1] = { 0.0f, 1.0f, 0.0f }; // Y軸
    obb.orientations[2] = { 0.0f, 0.0f, 1.0f }; // Z軸
    obb.size = { 1.0f, 1.0f, 0.5f }; // 各軸方向の半サイズ
    obb.color = static_cast<int>(Color::WHITE); // 色の初期値
}

bool OBBCollider::Dispatch(Collider* other) {
    return other->Action(this);
}

void OBBCollider::Draw()
{
    DrawLine::GetInstance()->DrawOBB(obb);
}

Vector3 OBBCollider::GetCenter() const
{
    return obb.center;
}

void OBBCollider::SetPosition(const Vector3& position)
{
    obb.center = position;
}

bool OBBCollider::Action(SphereCollider* other) {
    return CheckSphereVsOBB(other->sphere, obb);
}

bool OBBCollider::Action(AABBCollider* other) {
    return CheckAABBVsOBB(other->aabb, obb);
}

bool OBBCollider::Action(OBBCollider* other) {
    return CheckOBBVsOBB(obb, other->obb);
}

bool OBBCollider::Action(CapsuleCollider* other) {
    return CheckOBBVsCapsule(obb, other->capsule);
}

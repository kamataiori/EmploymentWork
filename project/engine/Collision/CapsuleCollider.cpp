#include "CapsuleCollider.h"
#include "CollisionFunctions.h"
#include "SphereCollider.h"
#include "AABBCollider.h"
#include "OBBCollider.h"

CapsuleCollider::CapsuleCollider(const Capsule& cap) : capsule(cap) {
	capsule.start = { 1.6f, 0.0f, 0.0f };
	capsule.end = { 1.6f, -1.5f, 0.0f };
	capsule.radius = 0.5f;
	capsule.color = static_cast<int>(Color::WHITE);
	capsule.segments = 16; // 円周を構成する分割数
	capsule.rings = 8;     // 球部分を構成する分割数
}

bool CapsuleCollider::Dispatch(Collider* other) {
	return	other->Action(this);
}

void CapsuleCollider::Draw()
{
	DrawLine::GetInstance()->DrawCapsule(capsule);
}

bool CapsuleCollider::Action(SphereCollider* other) {
	return CheckSphereVsCapsule(other->sphere, capsule);
}

bool CapsuleCollider::Action(AABBCollider* other) {
	return CheckAABBVsCapsule(other->aabb, capsule);
}

bool CapsuleCollider::Action(OBBCollider* other) {
	return CheckOBBVsCapsule(other->obb, capsule);
}

bool CapsuleCollider::Action(CapsuleCollider* other) {
	return CheckCapsuleVsCapsule(capsule, other->capsule);
}

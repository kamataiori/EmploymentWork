#include "AABBCollider.h"
#include "CollisionFunctions.h"
#include "SphereCollider.h"
#include "OBBCollider.h"
#include "CapsuleCollider.h"

AABBCollider::AABBCollider(const AABB& box) : aabb(box) {
    aabb.min = { -1.8f, 2.2f, 3.0f }; // AABB の最小点を少し下げる
    aabb.max = { 1.8f, 1.1f, 0.5f };  // AABB の最大点を少し上げる
    aabb.color = static_cast<int>(Color::WHITE); // AABBの色を赤に設定
}

bool AABBCollider::Dispatch(Collider* other) {
    return other->Action(this);
}

void AABBCollider::Draw()
{
    DrawLine::GetInstance()->DrawAABB(aabb);
}

bool AABBCollider::Action(SphereCollider* other) {
    return CheckSphereVsAABB(other->sphere, aabb);
}

bool AABBCollider::Action(AABBCollider* other) {
    return CheckAABBVsAABB(aabb, other->aabb);
}

bool AABBCollider::Action(OBBCollider* other) {
    return CheckAABBVsOBB(aabb, other->obb);
}

bool AABBCollider::Action(CapsuleCollider* other) {
    return CheckAABBVsCapsule(aabb, other->capsule);
}

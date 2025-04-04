#pragma once
#include "Struct.h"

// 衝突判定関数 (グローバル関数)

// SphereとSphere
bool CheckSphereVsSphere(const Sphere& s1, const Sphere& s2);
// SphereとAABB
bool CheckSphereVsAABB(const Sphere& sphere, const AABB& aabb);
// SphereとOBB
bool CheckSphereVsOBB(const Sphere& sphere, const OBB& obb);
// Sphereとカプセル
bool CheckSphereVsCapsule(const Sphere& sphere, const Capsule& capsule);
// AABBとAABB
bool CheckAABBVsAABB(const AABB& aabb1, const AABB& aabb2);
// AABBとOBB
bool CheckAABBVsOBB(const AABB& aabb, const OBB& obb);
// AABBとカプセル
bool CheckAABBVsCapsule(const AABB& aabb, const Capsule& capsule);
// OBBとOBB
bool CheckOBBVsOBB(const OBB& obb1, const OBB& obb2);
// OBBとカプセル
bool CheckOBBVsCapsule(const OBB& obb, const Capsule& capsule);
// カプセルとカプセル
bool CheckCapsuleVsCapsule(const Capsule& capsule1, const Capsule& capsule2);

void ClosestPointSegmentToSegment(
	const Vector3& p1, const Vector3& q1, // 線分Aの両端
	const Vector3& p2, const Vector3& q2, // 線分Bの両端
	Vector3& c1, Vector3& c2);

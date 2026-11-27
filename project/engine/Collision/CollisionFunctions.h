#pragma once
#include "Struct.h"

// 接触情報（押し出し用）。
//  normal : 第1引数の形状を第2引数（三角形など）から押し出す向き（正規化）
//  depth  : めり込み量（>0）
struct Contact {
    bool    hit = false;
    Vector3 normal{ 0.0f, 0.0f, 0.0f };
    float   depth = 0.0f;
};

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

// ------- 三角形（メッシュBVH）との判定 -------

// 点 p から三角形上への最近接点
Vector3 ClosestPointOnTriangle(const Vector3& p, const Triangle& tri);

// 三角形 vs 各形状（交差の有無だけ）
bool CheckSphereVsTriangle(const Sphere& s, const Triangle& t);
bool CheckCapsuleVsTriangle(const Capsule& c, const Triangle& t);
bool CheckAABBVsTriangle(const AABB& a, const Triangle& t);
bool CheckOBBVsTriangle(const OBB& o, const Triangle& t);

// 三角形との接触（押し出し用）。normal は形状を三角形から押し出す向き、depth>0。
bool SphereVsTriangleContact(const Sphere& s, const Triangle& t, Contact& out);
bool CapsuleVsTriangleContact(const Capsule& c, const Triangle& t, Contact& out);

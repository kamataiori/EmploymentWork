#include "CollisionFunctions.h"
#include <cmath>
#include "MathFunctions.h"
#include <list>

bool CheckSphereVsSphere(const Sphere& s1, const Sphere& s2)
{
	float distSq = LengthSq(s1.center - s2.center);
	float radiusSum = s1.radius + s2.radius;
	return distSq <= (radiusSum * radiusSum);
}

bool CheckSphereVsAABB(const Sphere& sphere, const AABB& aabb)
{
	Vector3 closestPoint = Clamp(sphere.center, aabb.min, aabb.max);
	return LengthSq(sphere.center - closestPoint) <= (sphere.radius * sphere.radius);
}

bool CheckSphereVsOBB(const Sphere& sphere, const OBB& obb)
{
    Vector3 closestPoint = obb.center;

    // OBB の3軸方向に対して最近接点を求める
    for (int i = 0; i < 3; i++) {
        float dist = Dot(sphere.center - obb.center, obb.orientations[i]);

        // dist を -size.x ~ size.x に制限する
        if (i == 0) {
            dist = Clamp(dist, -obb.size.x, obb.size.x);
        }
        else if (i == 1) {
            dist = Clamp(dist, -obb.size.y, obb.size.y);
        }
        else {
            dist = Clamp(dist, -obb.size.z, obb.size.z);
        }

        closestPoint += obb.orientations[i] * dist;
    }

    return LengthSq(sphere.center - closestPoint) <= (sphere.radius * sphere.radius);
}

bool CheckSphereVsCapsule(const Sphere& sphere, const Capsule& capsule)
{
    Vector3 capsuleAxis = capsule.end - capsule.start;
    float t = Dot(sphere.center - capsule.start, Normalize(capsuleAxis)) / Length(capsuleAxis);
    t = Clamp(t, 0.0f, 1.0f);
    Vector3 closestPoint = capsule.start + capsuleAxis * t;
    return DistanceSq(sphere.center, closestPoint) <= (sphere.radius + capsule.radius) * (sphere.radius + capsule.radius);
}

bool CheckAABBVsAABB(const AABB& aabb1, const AABB& aabb2)
{
    return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
        (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
        (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
}

bool CheckAABBVsOBB(const AABB& aabb, const OBB& obb)
{
    // AABBの中心とサイズ
    Vector3 aabbCenter = (aabb.min + aabb.max) * 0.5f;
    Vector3 aabbHalfSize = (aabb.max - aabb.min) * 0.5f;

    // OBB の3軸
    Vector3 axes[15];

    // AABB の3軸 (ワールド空間ではX, Y, Z方向)
    axes[0] = Vector3{ 1.0f, 0.0f, 0.0f };
    axes[1] = Vector3{ 0.0f, 1.0f, 0.0f };
    axes[2] = Vector3{ 0.0f, 0.0f, 1.0f };

    // OBB の3軸
    for (int i = 0; i < 3; i++) {
        axes[i + 3] = obb.orientations[i];
    }

    // 軸の外積 (9軸)
    int index = 6;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            axes[index++] = Normalize(Cross(axes[i], axes[j + 3]));
        }
    }

    Vector3 distanceVec = obb.center - aabbCenter;

    for (int i = 0; i < 15; i++) {
        float projectionAABB = fabs(Dot(aabbHalfSize, axes[i]));
        float projectionOBB = fabs(Dot(obb.orientations[0] * obb.size.x, axes[i])) +
            fabs(Dot(obb.orientations[1] * obb.size.y, axes[i])) +
            fabs(Dot(obb.orientations[2] * obb.size.z, axes[i]));
        float centerDist = fabs(Dot(distanceVec, axes[i]));

        if (centerDist > projectionAABB + projectionOBB) {
            return false;
        }
    }

    return true;
}

bool CheckAABBVsCapsule(const AABB& aabb, const Capsule& capsule)
{
    // カプセルの線分の最近接点を求める
    Vector3 closestPoint;
    Vector3 capsuleAxis = capsule.end - capsule.start;

    float t = Dot(aabb.min - capsule.start, Normalize(capsuleAxis)) / Length(capsuleAxis);
    t = Clamp(t, 0.0f, 1.0f);
    closestPoint = capsule.start + capsuleAxis * t;

    // AABB の最近接点を取得
    Vector3 closestAABBPoint = Clamp(closestPoint, aabb.min, aabb.max);

    // カプセルの半径以内なら衝突
    return LengthSq(closestAABBPoint - closestPoint) <= (capsule.radius * capsule.radius);
}

bool CheckOBBVsOBB(const OBB& obb1, const OBB& obb2)
{
    Vector3 axes[15];

    for (int i = 0; i < 3; ++i) {
        axes[i] = obb1.orientations[i];
        axes[i + 3] = obb2.orientations[i];
    }

    int index = 6;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            axes[index++] = Normalize(Cross(obb1.orientations[i], obb2.orientations[j]));
        }
    }

    Vector3 distanceVec = obb2.center - obb1.center;
    for (int i = 0; i < 15; ++i) {
        float projection1 = ProjectOBBOnAxis(obb1, axes[i]);
        float projection2 = ProjectOBBOnAxis(obb2, axes[i]);
        float centerDist = fabs(Dot(distanceVec, axes[i]));

        if (centerDist > projection1 + projection2) {
            return false;
        }
    }
    return true;
}

bool CheckOBBVsCapsule(const OBB& obb, const Capsule& capsule)
{
    // カプセルの軸の最近接点を求める
    Vector3 closestPoint;
    Vector3 capsuleAxis = capsule.end - capsule.start;

    float t = Dot(obb.center - capsule.start, Normalize(capsuleAxis)) / Length(capsuleAxis);
    t = Clamp(t, 0.0f, 1.0f);
    closestPoint = capsule.start + capsuleAxis * t;

    // OBB 内に最近接点があるかチェック
    Vector3 localPoint = closestPoint - obb.center;

    for (int i = 0; i < 3; i++) {
        float dist = Dot(localPoint, obb.orientations[i]);

        // 修正: obb.size[i] の代わりに obb.size.x, y, z を使用
        float halfExtent = (i == 0) ? obb.size.x : (i == 1) ? obb.size.y : obb.size.z;

        if (fabs(dist) > halfExtent) return false;
    }

    return true;
}

bool CheckCapsuleVsCapsule(const Capsule& capsule1, const Capsule& capsule2)
{
    Vector3 dir1 = Normalize(capsule1.end - capsule1.start);
    Vector3 dir2 = Normalize(capsule2.end - capsule2.start);

    Vector3 closest1, closest2;
    ClosestPointSegmentToSegment(capsule1.start, capsule1.end, capsule2.start, capsule2.end, closest1, closest2);

    float distSq = LengthSq(closest1 - closest2);
    float radiusSum = capsule1.radius + capsule2.radius;
    return distSq <= (radiusSum * radiusSum); return false;
}

void ClosestPointSegmentToSegment(const Vector3& p1, const Vector3& q1, const Vector3& p2, const Vector3& q2, Vector3& c1, Vector3& c2)
{
    // 最近接点の出力
    Vector3 d1 = q1 - p1;  // 線分Aの方向
    Vector3 d2 = q2 - p2;  // 線分Bの方向
    Vector3 r = p1 - p2;

    float a = Dot(d1, d1); // |d1|^2
    float e = Dot(d2, d2); // |d2|^2
    float f = Dot(d2, r);

    float s, t; // 線分A, B 上の最近接点のパラメータ

    if (a <= 1e-6f && e <= 1e-6f) {
        // 両方とも1点 (線分がゼロ長)
        s = t = 0.0f;
        c1 = p1;
        c2 = p2;
        return;
    }

    if (a <= 1e-6f) {
        // 線分Aがゼロ長 (点)
        s = 0.0f;
        t = Clamp(f / e, 0.0f, 1.0f);
    }
    else {
        float c = Dot(d1, r);
        if (e <= 1e-6f) {
            // 線分Bがゼロ長 (点)
            t = 0.0f;
            s = Clamp(-c / a, 0.0f, 1.0f);
        }
        else {
            float b = Dot(d1, d2);
            float denom = a * e - b * b;

            if (denom != 0.0f) {
                s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            }
            else {
                s = 0.0f;
            }

            t = (b * s + f) / e;
            t = Clamp(t, 0.0f, 1.0f);
        }
    }

    s = Clamp(s, 0.0f, 1.0f);
    t = Clamp(t, 0.0f, 1.0f);

    // 計算したパラメータ `s`, `t` に基づき、最近接点を決定
    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
}

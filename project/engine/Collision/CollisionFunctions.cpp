#include "CollisionFunctions.h"
#include <cmath>
#include "MathFunctions.h"
#include <list>
#include <algorithm>
#include <limits>

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

// ===================================================================
// 三角形（メッシュBVH）との判定
// ===================================================================

// 点 p から三角形上への最近接点（Ericson "Real-Time Collision Detection"）
Vector3 ClosestPointOnTriangle(const Vector3& p, const Triangle& tri)
{
    const Vector3& a = tri.vertices[0];
    const Vector3& b = tri.vertices[1];
    const Vector3& c = tri.vertices[2];

    const Vector3 ab = b - a;
    const Vector3 ac = c - a;
    const Vector3 ap = p - a;
    const float d1 = Dot(ab, ap);
    const float d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a; // 頂点a領域

    const Vector3 bp = p - b;
    const float d3 = Dot(ab, bp);
    const float d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b; // 頂点b領域

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        return a + ab * v; // 辺ab上
    }

    const Vector3 cp = p - c;
    const float d5 = Dot(ab, cp);
    const float d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c; // 頂点c領域

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        return a + ac * w; // 辺ac上
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w; // 辺bc上
    }

    // 面内：重心座標で復元
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return a + ab * v + ac * w;
}

bool CheckSphereVsTriangle(const Sphere& s, const Triangle& t)
{
    const Vector3 cp = ClosestPointOnTriangle(s.center, t);
    return LengthSq(s.center - cp) <= s.radius * s.radius;
}

bool SphereVsTriangleContact(const Sphere& s, const Triangle& t, Contact& out)
{
    const Vector3 cp = ClosestPointOnTriangle(s.center, t);
    const Vector3 d = s.center - cp;
    const float distSq = LengthSq(d);
    if (distSq > s.radius * s.radius) { out.hit = false; return false; }

    const float dist = std::sqrt(distSq);
    out.hit = true;
    if (dist > 1e-6f) {
        out.normal = d / dist;
    }
    else {
        // 中心が面上にある：三角形法線で押し出す
        const Vector3 n = Cross(t.vertices[1] - t.vertices[0], t.vertices[2] - t.vertices[0]);
        out.normal = Normalize(n);
    }
    out.depth = s.radius - dist;
    return true;
}

// 線分(p,q) と三角形の最近接点ペアを求める（端点2＋各辺3の候補の最小）
static void ClosestPtSegmentTriangle(const Vector3& p, const Vector3& q,
    const Triangle& tri, Vector3& segPt, Vector3& triPt)
{
    float bestSq = std::numeric_limits<float>::max();

    // 候補1,2：線分の端点 → 三角形
    {
        const Vector3 ct = ClosestPointOnTriangle(p, tri);
        const float dsq = LengthSq(p - ct);
        if (dsq < bestSq) { bestSq = dsq; segPt = p; triPt = ct; }
    }
    {
        const Vector3 ct = ClosestPointOnTriangle(q, tri);
        const float dsq = LengthSq(q - ct);
        if (dsq < bestSq) { bestSq = dsq; segPt = q; triPt = ct; }
    }

    // 候補3-5：線分 vs 三角形の各辺
    for (int e = 0; e < 3; ++e) {
        const Vector3& a = tri.vertices[e];
        const Vector3& b = tri.vertices[(e + 1) % 3];
        Vector3 c1, c2;
        ClosestPointSegmentToSegment(p, q, a, b, c1, c2);
        const float dsq = LengthSq(c1 - c2);
        if (dsq < bestSq) { bestSq = dsq; segPt = c1; triPt = c2; }
    }
}

bool CheckCapsuleVsTriangle(const Capsule& c, const Triangle& t)
{
    Vector3 segPt, triPt;
    ClosestPtSegmentTriangle(c.start, c.end, t, segPt, triPt);
    return LengthSq(segPt - triPt) <= c.radius * c.radius;
}

bool CapsuleVsTriangleContact(const Capsule& c, const Triangle& t, Contact& out)
{
    Vector3 segPt, triPt;
    ClosestPtSegmentTriangle(c.start, c.end, t, segPt, triPt);
    const Vector3 d = segPt - triPt;
    const float distSq = LengthSq(d);
    if (distSq > c.radius * c.radius) { out.hit = false; return false; }

    const float dist = std::sqrt(distSq);
    out.hit = true;
    if (dist > 1e-6f) {
        out.normal = d / dist;
    }
    else {
        // 芯線が面と交差：三角形法線を、線分中点のある側へ向ける
        const Vector3 n = Normalize(Cross(t.vertices[1] - t.vertices[0], t.vertices[2] - t.vertices[0]));
        const Vector3 mid = (c.start + c.end) * 0.5f;
        out.normal = (Dot(mid - triPt, n) < 0.0f) ? (n * -1.0f) : n;
    }
    out.depth = c.radius - dist;
    return true;
}

// AABB(中心c・半径e) vs 三角形（Akenine-Möller の分離軸判定）
static bool AABBTriSAT(const Vector3& c, const Vector3& e, const Triangle& tri)
{
    const Vector3 v0 = tri.vertices[0] - c;
    const Vector3 v1 = tri.vertices[1] - c;
    const Vector3 v2 = tri.vertices[2] - c;

    auto axisSep = [&](const Vector3& axis) -> bool {
        // このaxisで分離していれば true（=交差しない）
        const float p0 = Dot(v0, axis);
        const float p1 = Dot(v1, axis);
        const float p2 = Dot(v2, axis);
        const float r = e.x * std::fabs(axis.x) + e.y * std::fabs(axis.y) + e.z * std::fabs(axis.z);
        const float mn = std::min({ p0, p1, p2 });
        const float mx = std::max({ p0, p1, p2 });
        return (mn > r || mx < -r);
    };

    const Vector3 f[3] = { v1 - v0, v2 - v1, v0 - v2 }; // 三角形の辺
    const Vector3 u[3] = { {1,0,0}, {0,1,0}, {0,0,1} }; // 箱の軸

    // 9本：箱の軸 × 三角形の辺
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const Vector3 axis = Cross(u[i], f[j]);
            if (LengthSq(axis) < 1e-8f) continue; // 退化軸はスキップ
            if (axisSep(axis)) return false;
        }
    }
    // 3本：箱の面法線
    if (axisSep(u[0]) || axisSep(u[1]) || axisSep(u[2])) return false;
    // 1本：三角形の面法線
    const Vector3 n = Cross(f[0], f[1]);
    if (LengthSq(n) > 1e-12f && axisSep(n)) return false;

    return true;
}

bool CheckAABBVsTriangle(const AABB& a, const Triangle& t)
{
    const Vector3 c = (a.min + a.max) * 0.5f;
    const Vector3 e = (a.max - a.min) * 0.5f;
    return AABBTriSAT(c, e, t);
}

bool CheckOBBVsTriangle(const OBB& o, const Triangle& t)
{
    // 三角形をOBBローカルへ移すと、箱は原点中心・半径 o.size の AABB になる
    Triangle lt = t;
    for (int i = 0; i < 3; ++i) {
        const Vector3 d = t.vertices[i] - o.center;
        lt.vertices[i] = { Dot(d, o.orientations[0]), Dot(d, o.orientations[1]), Dot(d, o.orientations[2]) };
    }
    return AABBTriSAT({ 0,0,0 }, o.size, lt);
}

#include "ShapeIntersect.h"
#include "BVH/MeshBVH.h"
#include <algorithm>
#include <cmath>

// ------------------------------------------------------------------
// 既存4形状（Sphere / AABB / OBB / Capsule）どうしのディスパッチ
// ------------------------------------------------------------------
static bool SphereX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsSphere(A.sphere, B.sphere);
    case ShapeKind::AABB:    return CheckSphereVsAABB(A.sphere, B.aabb);
    case ShapeKind::OBB:     return CheckSphereVsOBB(A.sphere, B.obb);
    case ShapeKind::Capsule: return CheckSphereVsCapsule(A.sphere, B.capsule);
    default: return false;
    }
}
static bool AABBX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsAABB(B.sphere, A.aabb); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsAABB(A.aabb, B.aabb);
    case ShapeKind::OBB:     return CheckAABBVsOBB(A.aabb, B.obb);
    case ShapeKind::Capsule: return CheckAABBVsCapsule(A.aabb, B.capsule);
    default: return false;
    }
}
static bool OBBX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsOBB(B.sphere, A.obb); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsOBB(B.aabb, A.obb);  // 逆呼び
    case ShapeKind::OBB:     return CheckOBBVsOBB(A.obb, B.obb);
    case ShapeKind::Capsule: return CheckOBBVsCapsule(A.obb, B.capsule);
    default: return false;
    }
}
static bool CapsuleX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsCapsule(B.sphere, A.capsule); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsCapsule(B.aabb, A.capsule);   // 逆呼び
    case ShapeKind::OBB:     return CheckOBBVsCapsule(B.obb, A.capsule);
    case ShapeKind::Capsule: return CheckCapsuleVsCapsule(A.capsule, B.capsule);
    default: return false;
    }
}

// ------------------------------------------------------------------
// Mesh（BVH）まわりのユーティリティ
// ------------------------------------------------------------------

// BVH問い合わせ用に、非Mesh形状のワールドAABBを求める
static AABB ShapeWorldAABB(const Shape& s) {
    AABB box{};
    switch (s.kind) {
    case ShapeKind::Sphere: {
        const Vector3 r{ s.sphere.radius, s.sphere.radius, s.sphere.radius };
        box.min = s.sphere.center - r;
        box.max = s.sphere.center + r;
        break;
    }
    case ShapeKind::AABB:
        box.min = s.aabb.min; box.max = s.aabb.max;
        break;
    case ShapeKind::OBB: {
        const OBB& o = s.obb;
        const Vector3 e{
            std::fabs(o.orientations[0].x) * o.size.x + std::fabs(o.orientations[1].x) * o.size.y + std::fabs(o.orientations[2].x) * o.size.z,
            std::fabs(o.orientations[0].y) * o.size.x + std::fabs(o.orientations[1].y) * o.size.y + std::fabs(o.orientations[2].y) * o.size.z,
            std::fabs(o.orientations[0].z) * o.size.x + std::fabs(o.orientations[1].z) * o.size.y + std::fabs(o.orientations[2].z) * o.size.z,
        };
        box.min = o.center - e; box.max = o.center + e;
        break;
    }
    case ShapeKind::Capsule: {
        const Capsule& c = s.capsule;
        const Vector3 r{ c.radius, c.radius, c.radius };
        const Vector3 mn{ (std::min)(c.start.x, c.end.x), (std::min)(c.start.y, c.end.y), (std::min)(c.start.z, c.end.z) };
        const Vector3 mx{ (std::max)(c.start.x, c.end.x), (std::max)(c.start.y, c.end.y), (std::max)(c.start.z, c.end.z) };
        box.min = mn - r; box.max = mx + r;
        break;
    }
    default: break;
    }
    return box;
}

// 非Mesh形状 vs 三角形（bool）
static bool ShapeVsTriangle(const Shape& s, const Triangle& t) {
    switch (s.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsTriangle(s.sphere, t);
    case ShapeKind::AABB:    return CheckAABBVsTriangle(s.aabb, t);
    case ShapeKind::OBB:     return CheckOBBVsTriangle(s.obb, t);
    case ShapeKind::Capsule: return CheckCapsuleVsTriangle(s.capsule, t);
    default: return false; // Mesh vs Mesh は非対応
    }
}

// Mesh vs 非Mesh形状（bool）。BVHで候補三角形だけ精密判定する。
static bool MeshVsShapeBool(const Shape& mesh, const Shape& other) {
    if (!mesh.mesh || mesh.mesh->Empty()) return false;
    if (other.kind == ShapeKind::Mesh) return false;
    const AABB box = ShapeWorldAABB(other);
    return mesh.mesh->QueryOverlap(box,
        [&](const Triangle& tri) { return ShapeVsTriangle(other, tri); });
}

// Mesh vs Sphere の接触（最深の三角形を採用）。n は球をメッシュから押し出す向き。
static bool MeshVsSphereContact(const Shape& mesh, const Sphere& sp, Vector3& n, float& depth) {
    if (!mesh.mesh || mesh.mesh->Empty()) return false;
    const Vector3 r{ sp.radius, sp.radius, sp.radius };
    AABB box{}; box.min = sp.center - r; box.max = sp.center + r;

    bool found = false; float best = -1.0e9f; Vector3 bestN{};
    mesh.mesh->QueryOverlap(box, [&](const Triangle& tri) -> bool {
        Contact c;
        if (SphereVsTriangleContact(sp, tri, c) && c.depth > best) {
            best = c.depth; bestN = c.normal; found = true;
        }
        return false; // 全候補を走査（最深を採るため早期終了しない）
        });
    if (found) { n = bestN; depth = best; }
    return found;
}

// Mesh vs Capsule の接触（最深の三角形を採用）。n はカプセルを押し出す向き。
static bool MeshVsCapsuleContact(const Shape& mesh, const Capsule& cap, Vector3& n, float& depth) {
    if (!mesh.mesh || mesh.mesh->Empty()) return false;
    const Vector3 r{ cap.radius, cap.radius, cap.radius };
    const Vector3 mn{ (std::min)(cap.start.x, cap.end.x), (std::min)(cap.start.y, cap.end.y), (std::min)(cap.start.z, cap.end.z) };
    const Vector3 mx{ (std::max)(cap.start.x, cap.end.x), (std::max)(cap.start.y, cap.end.y), (std::max)(cap.start.z, cap.end.z) };
    AABB box{}; box.min = mn - r; box.max = mx + r;

    bool found = false; float best = -1.0e9f; Vector3 bestN{};
    mesh.mesh->QueryOverlap(box, [&](const Triangle& tri) -> bool {
        Contact c;
        if (CapsuleVsTriangleContact(cap, tri, c) && c.depth > best) {
            best = c.depth; bestN = c.normal; found = true;
        }
        return false;
        });
    if (found) { n = bestN; depth = best; }
    return found;
}

// ------------------------------------------------------------------
// 公開API
// ------------------------------------------------------------------
bool Intersects(const Shape& a, const Shape& b) {
    // Mesh はどちら側に来てもここで捌く
    if (a.kind == ShapeKind::Mesh) return MeshVsShapeBool(a, b);
    if (b.kind == ShapeKind::Mesh) return MeshVsShapeBool(b, a);

    switch (a.kind) {
    case ShapeKind::Sphere:  return SphereX(a, b);
    case ShapeKind::AABB:    return AABBX(a, b);
    case ShapeKind::OBB:     return OBBX(a, b);
    case ShapeKind::Capsule: return CapsuleX(a, b);
    default: return false;
    }
}

bool Intersects(const Shape& a, const Shape& b, Contact& out) {
    out.hit = false; out.depth = 0.0f; out.normal = { 0.0f, 0.0f, 0.0f };

    // Mesh × Sphere/Capsule のみ「押し出しに使える接触」を返す
    if (a.kind == ShapeKind::Mesh || b.kind == ShapeKind::Mesh) {
        const Shape& mesh = (a.kind == ShapeKind::Mesh) ? a : b;
        const Shape& other = (a.kind == ShapeKind::Mesh) ? b : a;

        Vector3 n{}; float depth = 0.0f; bool hit = false;
        if (other.kind == ShapeKind::Sphere)  hit = MeshVsSphereContact(mesh, other.sphere, n, depth);
        else if (other.kind == ShapeKind::Capsule) hit = MeshVsCapsuleContact(mesh, other.capsule, n, depth);
        else {
            // 接触計算は未対応（OBB/AABB等）→ 検出だけ返す
            out.hit = MeshVsShapeBool(mesh, other);
            return out.hit;
        }

        if (!hit) return false;

        // n は other（形状側）を押し出す向き。
        // out.normal は「a を b から押し出す向き」に合わせる。
        out.hit = true;
        out.depth = depth;
        out.normal = (a.kind == ShapeKind::Mesh) ? (n * -1.0f) : n;
        return true;
    }

    // それ以外の組み合わせは押し出し接触なし（検出のみ）
    out.hit = Intersects(a, b);
    return out.hit;
}

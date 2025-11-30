#include "ShapeIntersect.h"

static bool SphereX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsSphere(A.sphere, B.sphere);
    case ShapeKind::AABB:    return CheckSphereVsAABB(A.sphere, B.aabb);
    case ShapeKind::OBB:     return CheckSphereVsOBB(A.sphere, B.obb);
    case ShapeKind::Capsule: return CheckSphereVsCapsule(A.sphere, B.capsule);
    }
    return false;
}
static bool AABBX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsAABB(B.sphere, A.aabb); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsAABB(A.aabb, B.aabb);
    case ShapeKind::OBB:     return CheckAABBVsOBB(A.aabb, B.obb);
    case ShapeKind::Capsule: return CheckAABBVsCapsule(A.aabb, B.capsule);
    }
    return false;
}
static bool OBBX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsOBB(B.sphere, A.obb); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsOBB(B.aabb, A.obb);  // 逆呼び
    case ShapeKind::OBB:     return CheckOBBVsOBB(A.obb, B.obb);
    case ShapeKind::Capsule: return CheckOBBVsCapsule(A.obb, B.capsule);
    }
    return false;
}
static bool CapsuleX(const Shape& A, const Shape& B) {
    switch (B.kind) {
    case ShapeKind::Sphere:  return CheckSphereVsCapsule(B.sphere, A.capsule); // 逆呼び
    case ShapeKind::AABB:    return CheckAABBVsCapsule(B.aabb, A.capsule);   // 逆呼び
    case ShapeKind::OBB:     return CheckOBBVsCapsule(B.obb, A.capsule);
    case ShapeKind::Capsule: return CheckCapsuleVsCapsule(A.capsule, B.capsule);
    }
    return false;
}

bool Intersects(const Shape& a, const Shape& b) {
    switch (a.kind) {
    case ShapeKind::Sphere:  return SphereX(a, b);
    case ShapeKind::AABB:    return AABBX(a, b);
    case ShapeKind::OBB:     return OBBX(a, b);
    case ShapeKind::Capsule: return CapsuleX(a, b);
    }
    return false;
}

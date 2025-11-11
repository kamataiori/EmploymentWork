#include "MultiCollider.h"

void MultiCollider::Draw()
{
    for (const auto& s : shapes_) {
        switch (s.kind) {
        case ShapeKind::Sphere:  DrawLine::GetInstance()->DrawSphere(s.sphere); break;
        case ShapeKind::AABB:    DrawLine::GetInstance()->DrawAABB(s.aabb);     break;
        case ShapeKind::OBB:     DrawLine::GetInstance()->DrawOBB(s.obb);       break;
        case ShapeKind::Capsule: DrawLine::GetInstance()->DrawCapsule(s.capsule); break;
        }
    }
}

const OBB& MultiCollider::GetOBB(size_t index) const
{
    assert(index < shapes_.size());
    auto it = shapes_.cbegin();
    std::advance(it, index);
    assert(it->kind == ShapeKind::OBB);
    return it->obb;
}

Sphere& MultiCollider::MutableSphere(size_t index)
{
    assert(index < shapes_.size());
    auto it = shapes_.begin();
    std::advance(it, index);
    assert(it->kind == ShapeKind::Sphere); // 安全ガード
    return it->sphere;
}

const Sphere& MultiCollider::GetSphere(size_t index) const
{
    assert(index < shapes_.size());
    auto it = shapes_.cbegin();
    std::advance(it, index);
    assert(it->kind == ShapeKind::Sphere);
    return it->sphere;
}

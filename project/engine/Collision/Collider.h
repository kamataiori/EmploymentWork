#pragma once
#include <cstdint>
#include <vector>
#include "Struct.h"
#include "DrawLine.h"
#include "CollisionTypeIdDef.h"

enum class ShapeKind : uint8_t { Sphere, AABB, OBB, Capsule };

struct Shape {
    ShapeKind kind;
    // 値保持（簡単・局所的メモリのための by-value）
    Sphere  sphere{};
    AABB    aabb{};
    OBB     obb{};
    Capsule capsule{};
};

class Collider {
public:
    virtual ~Collider() = default;

    // 形状リスト公開（読み取り）
    virtual const std::vector<Shape>& GetShapes() const = 0;

    // デバッグ描画（任意）
    virtual void Draw() = 0;

    // 接触コールバック
    virtual void OnCollision() {}

    // 種別ID
    uint32_t GetTypeID() const { return typeID_; }
    void SetTypeID(uint32_t typeID) { typeID_ = typeID; }

private:
    uint32_t typeID_ = 0u;
};

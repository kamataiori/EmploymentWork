#pragma once
#include <cstdint>
#include <vector>
#include "Struct.h"
#include "DrawLine.h"
#include "CollisionTypeIdDef.h"

class Collider;

enum class ShapeKind : uint8_t { Sphere, AABB, OBB, Capsule };

struct Shape {
    ShapeKind kind;
    // 値保持（簡単・局所的メモリのための by-value）
    Sphere  sphere{};
    AABB    aabb{};
    OBB     obb{};
    Capsule capsule{};
};

struct CollisionInfo
{
    Collider* self = nullptr;
    Collider* other = nullptr;
    uint32_t selfType = 0;
    uint32_t otherType = 0;
    // 必要になったら後で: 接触点/法線/押し戻し量 なども足せる
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

    // 相手情報付き（デフォルトは旧OnCollisionへフォールバック）
    virtual void OnCollision(const CollisionInfo& info) { (void)info; OnCollision(); }

    // 種別ID
    uint32_t GetTypeID() const { return typeID_; }
    void SetTypeID(uint32_t typeID) { typeID_ = typeID; }

private:
    uint32_t typeID_ = 0u;
};

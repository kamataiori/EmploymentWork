#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "Struct.h"
#include "DrawLine.h"
#include "CollisionTypeIdDef.h"

class Collider;
class MeshBVH;

enum class ShapeKind : uint8_t { Sphere, AABB, OBB, Capsule, Mesh };

// 衝突応答の種類
//  Trigger : 通知（OnCollision）だけ。すり抜ける。従来どおり。
//  Blocking: すり抜けない。Blocking同士は押し戻し（MTV適用）の対象になる。
enum class CollisionResponse : uint8_t { Trigger, Blocking };

struct Shape {
    ShapeKind kind;
    // 値保持（簡単・局所的メモリのための by-value）
    Sphere  sphere{};
    AABB    aabb{};
    OBB     obb{};
    Capsule capsule{};
    // Mesh用：BVHは重いので「共有・不変」ポインタで持つ（by-valueコピーが軽い）。
    // 静的ステージはワールド空間へ焼き込んで1回だけ構築し使い回す。
    std::shared_ptr<const MeshBVH> mesh;
};

struct CollisionInfo
{
    Collider* self = nullptr;
    Collider* other = nullptr;
    uint32_t selfType = 0;
    uint32_t otherType = 0;
    // 接触法線（self を押し出す向き）とめり込み量。対応する形状ペアでのみ有効。
    Vector3 normal{ 0.0f, 0.0f, 0.0f };
    float depth = 0.0f;
};

class Collider {
public:

    Collider();
    virtual ~Collider() = default;

    // 多段ヒット回避用
    uint32_t GetInstanceId() const { return instanceId_; }

    // 形状リスト公開（読み取り）
    virtual const std::vector<Shape>& GetShapes() const = 0;

    // デバッグ描画（任意）
    virtual void Draw() = 0;

    // 接触コールバック
    virtual void OnCollision() {}

    // 相手情報付き（デフォルトは旧OnCollisionへフォールバック）
    virtual void OnCollision(const CollisionInfo& info) { (void)info; OnCollision(); }

    // 押し戻し（MTV）の適用。Blocking同士の重なり時に CollisionManager から
    // 毎フレーム（非ゲート）で呼ばれる。派生側で自身の位置を mtv だけ動かす。
    virtual void ApplyPushOut(const Vector3& mtv) { (void)mtv; }

    // 種別ID
    uint32_t GetTypeID() const { return typeID_; }
    void SetTypeID(uint32_t typeID) { typeID_ = typeID; }

    // 衝突応答（Trigger / Blocking）
    CollisionResponse GetResponse() const { return response_; }
    void SetResponse(CollisionResponse r) { response_ = r; }

    // 押し戻しで動かせるか（静的ステージは false）
    bool IsMovable() const { return movable_; }
    void SetMovable(bool m) { movable_ = m; }

private:
    uint32_t typeID_ = 0u;
    uint32_t instanceId_ = 0;
    CollisionResponse response_ = CollisionResponse::Trigger;
    bool movable_ = true;
};

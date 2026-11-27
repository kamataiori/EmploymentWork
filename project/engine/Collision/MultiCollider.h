#pragma once
#include "Collider.h"
#include <functional>

// 形状コライダー
class MultiCollider final : public Collider {
public:

    MultiCollider() = default;

    // コンストラクタで最初の形状を一つ追加
    explicit MultiCollider(const Shape& first) { shapes_.push_back(first); }

    // ヒット時に呼ぶコールバックを登録
    void SetHitCallback(std::function<void()> cb) { onHit_ = std::move(cb); }

    // 相手情報付き
    void SetHitCallbackEx(std::function<void(const CollisionInfo&)> cb) { onHitEx_ = std::move(cb); }

    // 押し戻し（MTV）を受け取るコールバックを登録
    void SetPushOutCallback(std::function<void(const Vector3&)> cb) { onPushOut_ = std::move(cb); }

    // Blocking同士の重なり時に呼ばれる。登録があれば MTV を渡す。
    void ApplyPushOut(const Vector3& mtv) override { if (onPushOut_) onPushOut_(mtv); }

    // 橋渡し
    void OnCollision() override { if (onHit_) onHit_(); }

    // 相手情報付き（優先してExを呼ぶ → 無ければ旧へ）
    void OnCollision(const CollisionInfo& info) override
    {
        if (onHitEx_) { onHitEx_(info); return; }
        OnCollision();
    }

    // 追加API（任意タイミングで形状を足せる）
    void Add(const Shape& s) { shapes_.push_back(s); }

    // ユーティリティ（個別追加）
    void AddSphere(const Sphere& s) { Shape sh{ ShapeKind::Sphere };   sh.sphere = s; shapes_.push_back(sh); }
    void AddAABB(const AABB& a) { Shape sh{ ShapeKind::AABB };     sh.aabb = a; shapes_.push_back(sh); }
    void AddOBB(const OBB& o) { Shape sh{ ShapeKind::OBB };      sh.obb = o; shapes_.push_back(sh); }
    void AddCapsule(const Capsule& c) { Shape sh{ ShapeKind::Capsule };  sh.capsule = c; shapes_.push_back(sh); }
    void AddMesh(std::shared_ptr<const MeshBVH> bvh) { Shape sh{ ShapeKind::Mesh }; sh.mesh = std::move(bvh); shapes_.push_back(sh); }

    // 上書き・クリア
    void Clear() { shapes_.clear(); }

    // 形状アクセス
    const std::vector<Shape>& GetShapes() const override { return shapes_; }

    // デバッグ描画
    void Draw() override;

    // 毎フレーム OBB の center/axes を更新する
    OBB& MutableOBB(size_t idx) { return shapes_.at(idx).obb; }

    const OBB& GetOBB(size_t index) const;

    Sphere& MutableSphere(size_t index);
    const Sphere& GetSphere(size_t index) const;

    // 毎フレーム Capsule の start/end/radius を更新する
    Capsule& MutableCapsule(size_t index) { return shapes_.at(index).capsule; }

private:
    std::vector<Shape> shapes_;
    std::function<void()> onHit_;
    std::function<void(const CollisionInfo&)> onHitEx_;
    std::function<void(const Vector3&)> onPushOut_;
};


// 形状コライダー
#pragma once
#include "Collider.h"

class MultiCollider final : public Collider {
public:
    // コンストラクタで最初の形状を一つ追加
    explicit MultiCollider(const Shape& first) { shapes_.push_back(first); }

    // 追加API（任意タイミングで形状を足せる）
    void Add(const Shape& s) { shapes_.push_back(s); }

    // ユーティリティ（個別追加）
    void AddSphere(const Sphere& s) { Shape sh{ ShapeKind::Sphere };   sh.sphere = s; shapes_.push_back(sh); }
    void AddAABB(const AABB& a) { Shape sh{ ShapeKind::AABB };     sh.aabb = a; shapes_.push_back(sh); }
    void AddOBB(const OBB& o) { Shape sh{ ShapeKind::OBB };      sh.obb = o; shapes_.push_back(sh); }
    void AddCapsule(const Capsule& c) { Shape sh{ ShapeKind::Capsule };  sh.capsule = c; shapes_.push_back(sh); }

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

private:
    std::vector<Shape> shapes_;
};


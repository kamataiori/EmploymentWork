#pragma once

#include "Struct.h"
#include <list>
#include <algorithm>
#include "CollisionTypeIdDef.h"

class Collider;

class CollisionManager {
public:
    // コライダーの登録
    void RegisterCollider(Collider* collider);

    // コライダーの登録解除
    void UnregisterCollider(Collider* collider);

    // 全てのコライダー登録をクリア
    void Reset();

    // 登録された全てのコライダー間で衝突チェック
    void CheckAllCollisions();

private:
    // 衝突判定を無視すべきタイプの組み合わせを確認
    bool ShouldIgnoreCollision(uint32_t type1, uint32_t type2);

    // 例外：同グループ内でも「当てたい」ペア
    bool IsForceCollide(uint32_t type1, uint32_t type2) const;

private:
    // 登録されたコライダーのリスト
    std::list<Collider*> colliders;
};

// ---------- CollisionTypeIdDef をハッシュ可能にする ----------
namespace std {
    template <>
    struct hash<CollisionTypeIdDef> {
        size_t operator()(const CollisionTypeIdDef& type) const {
            return std::hash<std::underlying_type_t<CollisionTypeIdDef>>{}(
                static_cast<std::underlying_type_t<CollisionTypeIdDef>>(type)
                );
        }
    };
}
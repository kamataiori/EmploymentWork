#pragma once
#include "Struct.h"
#include <list>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <utility>
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
    bool ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2);

    // 例外：同グループ内でも「当てたい」ペア
    bool IsForceCollide(CollisionTypeIdDef type1, CollisionTypeIdDef type2) const;

    // 接触継続（Stay）で何秒おきにヒットを許可するか（Typeペア→秒）
    float GetStayCooldown(CollisionTypeIdDef a, CollisionTypeIdDef b) const;

    // 
    static uint64_t MakePairKey(uint32_t a, uint32_t b)
    {
        if (a > b) std::swap(a, b);
        return (uint64_t(a) << 32) | uint64_t(b);
    }

private:
    // 登録されたコライダーのリスト
    std::list<Collider*> colliders;

    std::unordered_set<uint64_t> prevContacts_;
    std::unordered_set<uint64_t> currContacts_;

    // 内部時刻（秒）
    float nowSec_ = 0.0f;
    // 接触中の再ヒット用：最後にヒットした時刻（ペアごと）
    std::unordered_map<uint64_t, float> lastHitTime_;
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
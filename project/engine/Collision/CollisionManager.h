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
    bool ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2);

    // pairをハッシュ可能にするための構造体（unordered_setで使う）
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
        }
    };

private:
    // 登録されたコライダーのリスト
    std::list<Collider*> colliders;
};

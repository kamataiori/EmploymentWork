#include "CollisionManager.h"
#include "Collider.h"
#include <unordered_set>

// コライダーを登録
void CollisionManager::RegisterCollider(Collider* collider) {
    colliders.push_back(collider);
}

// コライダーを登録解除
void CollisionManager::UnregisterCollider(Collider* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
}

// コライダーを全削除
void CollisionManager::Reset() {
    colliders.clear();
}

// 登録された全てのコライダーの組み合わせで衝突チェック
void CollisionManager::CheckAllCollisions() {
    for (auto it1 = colliders.begin(); it1 != colliders.end(); ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != colliders.end(); ++it2) {
            CollisionTypeIdDef id1 = static_cast<CollisionTypeIdDef>((*it1)->GetTypeID());
            CollisionTypeIdDef id2 = static_cast<CollisionTypeIdDef>((*it2)->GetTypeID());

            // 特定の組み合わせは衝突チェックをスキップ
            if (ShouldIgnoreCollision(id1, id2)) {
                continue;
            }

            // 衝突判定
            if ((*it1)->Dispatch(*it2)) {
                (*it1)->OnCollision();
                (*it2)->OnCollision();
            }
        }
    }
    Reset();
}

// 衝突を無視するペアをチェック
bool CollisionManager::ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2) {
    static const std::unordered_set<std::pair<CollisionTypeIdDef, CollisionTypeIdDef>, pair_hash> ignoredPairs = {
        {CollisionTypeIdDef::kPlayer, CollisionTypeIdDef::kEnemy},
        {CollisionTypeIdDef::kEnemy, CollisionTypeIdDef::kPlayer},
        {CollisionTypeIdDef::PlayerBullet, CollisionTypeIdDef::kPlayer},
        { CollisionTypeIdDef::kPlayer, CollisionTypeIdDef::PlayerBullet}
        // 必要ならここに追加（例: 弾同士など）
    };

    return ignoredPairs.find({ type1, type2 }) != ignoredPairs.end();
}

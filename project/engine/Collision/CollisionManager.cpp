#include "CollisionManager.h"
#include "Collider.h"
#include <unordered_set>

void CollisionManager::RegisterCollider(Collider* collider) {
    colliders.push_back(collider);
}

void CollisionManager::UnregisterCollider(Collider* collider) {
    colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
}

void CollisionManager::Reset() {
    colliders.clear();
}

void CollisionManager::CheckAllCollisions() {
    for (auto it1 = colliders.begin(); it1 != colliders.end(); ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != colliders.end(); ++it2) {
            CollisionTypeIdDef id1 = static_cast<CollisionTypeIdDef>((*it1)->GetTypeID());
            CollisionTypeIdDef id2 = static_cast<CollisionTypeIdDef>((*it2)->GetTypeID());

            // 特定の組み合わせを無視
            if (ShouldIgnoreCollision(id1, id2)) {
                continue; // 当たり判定をスキップ
            }

            if ((*it1)->Dispatch(*it2)) {
                (*it1)->OnCollision();
                (*it2)->OnCollision();
            }
        }
    }
}

bool CollisionManager::ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2)
{
    static const std::unordered_set<std::pair<CollisionTypeIdDef, CollisionTypeIdDef>, pair_hash> ignoredPairs = {
        {CollisionTypeIdDef::kPlayer, CollisionTypeIdDef::kEnemy},
        {CollisionTypeIdDef::kEnemy, CollisionTypeIdDef::kPlayer},
        
    };

    return ignoredPairs.find({ type1, type2 }) != ignoredPairs.end();
}

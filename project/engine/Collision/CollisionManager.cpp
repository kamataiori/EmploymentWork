#include "CollisionManager.h"
#include "Collider.h"
#include "ShapeIntersect.h"
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
        auto it2 = it1; ++it2;
        for (; it2 != colliders.end(); ++it2) {
            CollisionTypeIdDef id1 = static_cast<CollisionTypeIdDef>((*it1)->GetTypeID());
            CollisionTypeIdDef id2 = static_cast<CollisionTypeIdDef>((*it2)->GetTypeID());

            if (ShouldIgnoreCollision(id1, id2)) { continue; } // 既存の無視表を活用

            const auto& s1 = (*it1)->GetShapes();
            const auto& s2 = (*it2)->GetShapes();

            bool hit = false;
            for (const auto& a : s1) {
                for (const auto& b : s2) {
                    if (Intersects(a, b)) { hit = true; break; }
                }
                if (hit) break;
            }
            if (hit) {
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
		{ CollisionTypeIdDef::kPlayer, CollisionTypeIdDef::PlayerBullet},

        {CollisionTypeIdDef::kPlayerWeapon, CollisionTypeIdDef::kPlayer},
        { CollisionTypeIdDef::kPlayer, CollisionTypeIdDef::kPlayerWeapon},

		{ CollisionTypeIdDef::kEnemy, CollisionTypeIdDef::EnemyAreaAttack },
	    { CollisionTypeIdDef::EnemyAreaAttack, CollisionTypeIdDef::kEnemy },

		{ CollisionTypeIdDef::kEnemy, CollisionTypeIdDef::EnemyBullet },
		{ CollisionTypeIdDef::EnemyBullet, CollisionTypeIdDef::kEnemy },

		{ CollisionTypeIdDef::EnemyAreaAttack, CollisionTypeIdDef::EnemyBullet },
		{ CollisionTypeIdDef::EnemyBullet, CollisionTypeIdDef::EnemyAreaAttack },

		{ CollisionTypeIdDef::EnemyAreaAttack, CollisionTypeIdDef::EnemyAreaAttack },
		{ CollisionTypeIdDef::EnemyBullet, CollisionTypeIdDef::EnemyBullet },
	    // 必要ならここに追加（例: playerとplayerBulletなど）
	};

	return ignoredPairs.find({ type1, type2 }) != ignoredPairs.end();
}

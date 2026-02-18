#include "CollisionManager.h"
#include "Collider.h"
#include "ShapeIntersect.h"
#include <unordered_set>
#include <engine/TimeManager.h>

// --------------------------
// 内部ユーティリティ
// --------------------------
static std::pair<uint32_t, uint32_t> NormalizeU32Pair(uint32_t a, uint32_t b)
{
	return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

struct u32pair_hash {
	size_t operator()(const std::pair<uint32_t, uint32_t>& p) const noexcept
	{
		// 簡易ハッシュ（十分）
		return (static_cast<size_t>(p.first) * 1315423911u) ^ static_cast<size_t>(p.second);
	}
};

// コライダーを登録
void CollisionManager::RegisterCollider(Collider* collider) {
	colliders.push_back(collider);
}

// コライダーを登録解除
void CollisionManager::UnregisterCollider(Collider* collider) {
	//colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
	colliders.remove(collider);
}

// コライダーを全削除
void CollisionManager::Reset() {
	colliders.clear();
}

// 登録された全てのコライダーの組み合わせで衝突チェック
void CollisionManager::CheckAllCollisions()
{
    // 時間を進める（TimeManagerの関数名はあなたの実装に合わせて）
    const float dt = TimeManager::GetInstance()->GetDeltaTime(); // ←違う名前ならここだけ置換
    nowSec_ += dt;

    currContacts_.clear();

    for (auto it1 = colliders.begin(); it1 != colliders.end(); ++it1) {
        auto it2 = it1; ++it2;
        for (; it2 != colliders.end(); ++it2) {

            auto* c1 = *it1;
            auto* c2 = *it2;

            auto id1 = static_cast<CollisionTypeIdDef>(c1->GetTypeID());
            auto id2 = static_cast<CollisionTypeIdDef>(c2->GetTypeID());

            if (ShouldIgnoreCollision(id1, id2)) { continue; }

            // ナローフェーズ
            const auto& s1 = c1->GetShapes();
            const auto& s2 = c2->GetShapes();

            bool hit = false;
            for (const auto& a : s1) {
                for (const auto& b : s2) {
                    if (Intersects(a, b)) { hit = true; break; }
                }
                if (hit) break;
            }
            if (!hit) continue;

            // ---- 多段ヒット抑止（Enter + StayCooldown）----
            const uint64_t key = MakePairKey(c1->GetInstanceId(), c2->GetInstanceId());
            currContacts_.insert(key);

            // 新規接触か？
            const bool isNewContact = (prevContacts_.find(key) == prevContacts_.end());

            // このTypeペアは“接触継続で再ヒット”を許可する？
            const float stayCd = GetStayCooldown(id1, id2);

            // 最後にヒットした時刻を取得（無ければ -inf 扱い）
            float last = -1e9f;
            auto itLast = lastHitTime_.find(key);
            if (itLast != lastHitTime_.end()) {
                last = itLast->second;
            }

            bool shouldFire = false;

            if (isNewContact)
            {
                // Enter：常に1回は当てる
                shouldFire = true;
            }
            else
            {
                // Stay：登録されているペアだけ、cooldown秒ごとに当てる
                if (stayCd > 0.0f)
                {
                    if ((nowSec_ - last) >= stayCd)
                    {
                        shouldFire = true;
                    }
                }
            }

            if (!shouldFire) {
                continue;
            }

            // ヒット時刻更新
            lastHitTime_[key] = nowSec_;

            // 新規/再ヒットのみ OnCollision
            CollisionInfo i1{ c1, c2, (uint32_t)id1, (uint32_t)id2 };
            CollisionInfo i2{ c2, c1, (uint32_t)id2, (uint32_t)id1 };
            c1->OnCollision(i1);
            c2->OnCollision(i2);

        }
    }

    // 次フレーム用
    prevContacts_.swap(currContacts_);

    // 接触が終わったペアの履歴を掃除（不要なら増え続けるので重要）
    for (auto it = lastHitTime_.begin(); it != lastHitTime_.end(); )
    {
        if (prevContacts_.find(it->first) == prevContacts_.end()) {
            it = lastHitTime_.erase(it);
        }
        else {
            ++it;
        }
    }

    // 毎フレーム登録を消す
    Reset();
}

// 衝突を無視するペアをチェック
bool CollisionManager::ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2) {
	// 例外（当てたい）なら無視しない
	if (IsForceCollide(type1, type2)) {
		return false;
	}

	const auto g1 = GetGroup(type1);
	const auto g2 = GetGroup(type2);

	// 基本ルール：同グループ内は衝突しない
	if (g1 == CollisionGroup::Player && g2 == CollisionGroup::Player) return true;
	if (g1 == CollisionGroup::Enemy && g2 == CollisionGroup::Enemy)  return true;

	// それ以外は衝突する
	return false;
}

bool CollisionManager::IsForceCollide(CollisionTypeIdDef type1, CollisionTypeIdDef type2) const
{
    // ここだけが「追加で書く場所」になる（同グループでも当てたい例外）
    static const std::unordered_set<std::pair<uint32_t, uint32_t>, u32pair_hash> forcePairs = {
        // 例：
        // NormalizeU32Pair((uint32_t)CollisionTypeIdDef::PlayerBullet, (uint32_t)CollisionTypeIdDef::kPlayerWeapon),
    };

    const uint32_t a = static_cast<uint32_t>(type1);
    const uint32_t b = static_cast<uint32_t>(type2);
    return forcePairs.find(NormalizeU32Pair(a, b)) != forcePairs.end();
}

float CollisionManager::GetStayCooldown(CollisionTypeIdDef a, CollisionTypeIdDef b) const
{
    // Normalize（順序違いを吸収）
    const uint32_t ua = static_cast<uint32_t>(a);
    const uint32_t ub = static_cast<uint32_t>(b);
    const auto key = (ua < ub) ? std::make_pair(ua, ub) : std::make_pair(ub, ua);

    // ここに「接触継続でも一定間隔で当てたい」ものだけ登録
    // 例：敵の範囲攻撃がプレイヤーに0.2秒ごとにダメージ
    //     EnemyAreaAttack × Player
    static const std::unordered_map<std::pair<uint32_t, uint32_t>, float, u32pair_hash> stayCooldown = {
        { NormalizeU32Pair((uint32_t)CollisionTypeIdDef::EnemyAreaAttack, (uint32_t)CollisionTypeIdDef::kPlayer), 0.20f },

        // 例：レーザー(仮) × Player を 0.10秒ごと
        // { NormalizeU32Pair((uint32_t)CollisionTypeIdDef::EnemyLaser, (uint32_t)CollisionTypeIdDef::kPlayer), 0.10f },
        { NormalizeU32Pair((uint32_t)CollisionTypeIdDef::kPlayerWeapon, (uint32_t)CollisionTypeIdDef::kEnemy), 1.2f },
    };

    auto it = stayCooldown.find(key);
    if (it == stayCooldown.end()) {
        return 0.0f; // 0なら“Enterのみ”（再ヒットなし）
    }
    return it->second;
}

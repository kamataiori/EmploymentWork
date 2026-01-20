#include "CollisionManager.h"
#include "Collider.h"
#include "ShapeIntersect.h"
#include <unordered_set>

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
void CollisionManager::CheckAllCollisions() {
	for (auto it1 = colliders.begin(); it1 != colliders.end(); ++it1)
	{
		auto it2 = it1;
		++it2;

		for (; it2 != colliders.end(); ++it2)
		{
			auto* c1 = *it1;
			auto* c2 = *it2;

			const uint32_t type1 = c1->GetTypeID();
			const uint32_t type2 = c2->GetTypeID();

			if (ShouldIgnoreCollision(type1, type2)) { continue; }

			const auto& s1 = c1->GetShapes();
			const auto& s2 = c2->GetShapes();

			bool hit = false;
			for (const auto& a : s1) {
				for (const auto& b : s2) {
					if (Intersects(a, b)) { hit = true; break; }
				}
				if (hit) break;
			}

			if (hit)
			{
				// 相手情報つき通知（Collider側が未対応なら OnCollision() にフォールバックする作りにしておく）
				CollisionInfo info1{};
				info1.self = c1;
				info1.other = c2;
				info1.selfType = type1;
				info1.otherType = type2;

				CollisionInfo info2{};
				info2.self = c2;
				info2.other = c1;
				info2.selfType = type2;
				info2.otherType = type1;

				c1->OnCollision(info1);
				c2->OnCollision(info2);
			}
		}
	}

	// あなたの現行設計：毎フレーム登録を消す
	Reset();
}

// 衝突を無視するペアをチェック
bool CollisionManager::ShouldIgnoreCollision(uint32_t type1, uint32_t type2) {
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

bool CollisionManager::IsForceCollide(uint32_t type1, uint32_t type2) const
{
	// ここだけが「追加で書く場所」になる
	// - 同グループ内は基本無視だが、ここに入れたペアだけ衝突させる
	// - NormalizeU32Pair なので片方向1行だけでOK
	//
	// 例：
	//  - Playerの弾が Playerの設置物 に当たる
	//  - Enemy弾が Enemyのバリア に当たる
	//
	// static const uint32_t PlayerTurret = MakeType(CollisionGroup::Player, 100); // 例
	// NormalizeU32Pair((uint32_t)CollisionTypeIdDef::PlayerBullet, PlayerTurret),

	static const std::unordered_set<std::pair<uint32_t, uint32_t>, u32pair_hash> forcePairs = {
		// いまは空でOK（＝同グループ内は全部無視）
	};

	return forcePairs.find(NormalizeU32Pair(type1, type2)) != forcePairs.end();
}

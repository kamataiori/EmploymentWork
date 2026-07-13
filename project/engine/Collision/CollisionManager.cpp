#include "CollisionManager.h"
#include "Collider.h"
#include "ShapeIntersect.h"
#include "BVH/MeshBVH.h"
#include "MathFunctions.h"
#include <unordered_set>
#include <string>
#include <Windows.h>
#include <engine/TimeManager.h>

// --------------------------
// 内部ユーティリティ
// --------------------------
static std::pair<uint32_t, uint32_t> NormalizeU32Pair(uint32_t a, uint32_t b)
{
	return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

// 高速移動によるすり抜け（トンネリング）を潰す。
// mover が前フレーム位置を持っていれば、前位置→現位置をスイープし、
// 壁を飛び越えていた場合は「最初に接触する位置」まで引き戻す。
// 引き戻した後のめり込みは、このあとの通常の離散押し戻しが解消する。
static void SweepBackIfTunneled(Collider* mover,
    const std::vector<Shape>& moverShapes,
    const std::vector<Shape>& stageShapes)
{
    if (!mover->IsMovable() || !mover->HasPrevCenter()) return;

    // 押し戻しプロキシは球1つを前提にしている
    if (moverShapes.empty() || moverShapes.front().kind != ShapeKind::Sphere) return;
    const Sphere& sp = moverShapes.front().sphere;

    for (const Shape& sh : stageShapes) {
        if (sh.kind != ShapeKind::Mesh) continue;

        Vector3 impact{};
        if (SweepSphereVsMesh(sh, mover->GetPrevCenter(), sp.center, sp.radius, impact)) {
            // 現在位置 → 接触位置 へ戻す（ApplyPushOut は相対移動量を受け取る）
            mover->ApplyPushOut(impact - sp.center);
            return;
        }
    }
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

            // Blocking同士は押し戻し用に最深接触も取りに行く
            const bool bothBlocking =
                (c1->GetResponse() == CollisionResponse::Blocking &&
                 c2->GetResponse() == CollisionResponse::Blocking);

            // ---- CCD：離散判定の前に、すり抜けていたら接触位置まで引き戻す ----
            // ここで位置を戻しておくと、以降の離散判定は「壁の手前にいる」状態で走るので
            // 通常どおり深さ付き接触が取れる。
            if (bothBlocking) {
                SweepBackIfTunneled(c1, s1, s2);
                SweepBackIfTunneled(c2, s2, s1);
            }

            bool hit = false;
            Contact deepest{};          // 最もめり込んでいる接触
            bool haveContact = false;

            for (const auto& a : s1) {
                for (const auto& b : s2) {
                    if (bothBlocking) {
                        Contact ct;
                        if (Intersects(a, b, ct)) {
                            hit = true;
                            if (ct.hit && ct.depth > deepest.depth) {
                                deepest = ct;
                                haveContact = true;
                            }
                        }
                    }
                    else {
                        if (Intersects(a, b)) { hit = true; break; }
                    }
                }
                if (hit && !bothBlocking) break; // 検出だけで良いなら即抜け
            }

            // [診断ログ] World(Stage)が絡むBlockingペアの評価結果。
            // BVHの三角形数・ルート境界と、プローブ球から最寄り三角形までの距離を出す。
            //  ・triCount=0        → 焼き込み失敗（BVHが空）
            //  ・nearDist が radius より十分大きいまま貫通 → 三角形が別の場所にある（変換ズレ）
            //  ・nearDist < radius なのに hit=0 → ナローフェーズの不具合
            // 壁に近いフレームは throttle せず毎フレーム出す（貫通の瞬間を取りこぼさないため）。
            {
                const bool stagePair =
                    (GetGroup(id1) == CollisionGroup::World || GetGroup(id2) == CollisionGroup::World);
                if (stagePair && bothBlocking) {
                    const bool worldIsFirst = (GetGroup(id1) == CollisionGroup::World);
                    const auto& stageShapes = worldIsFirst ? s1 : s2;
                    const auto& probeShapes = worldIsFirst ? s2 : s1;

                    // ステージ側のBVH
                    const MeshBVH* bvh = nullptr;
                    for (const Shape& sh : stageShapes) {
                        if (sh.kind == ShapeKind::Mesh && sh.mesh) { bvh = sh.mesh.get(); break; }
                    }
                    // プローブ側の球
                    Vector3 p{ 0.0f, 0.0f, 0.0f };
                    float radius = 0.0f;
                    bool haveSphere = false;
                    if (!probeShapes.empty() && probeShapes.front().kind == ShapeKind::Sphere) {
                        p = probeShapes.front().sphere.center;
                        radius = probeShapes.front().sphere.radius;
                        haveSphere = true;
                    }

                    // 最寄り三角形までの距離を測る（半径+探索マージンの箱で候補を集める）
                    const float kProbeMargin = 5.0f;
                    float nearDist = -1.0f;   // -1 = 候補ゼロ
                    int   candidates = 0;
                    if (bvh && haveSphere && !bvh->Empty()) {
                        const float r = radius + kProbeMargin;
                        AABB box{};
                        box.min = p - Vector3{ r, r, r };
                        box.max = p + Vector3{ r, r, r };
                        float best = 1.0e9f;
                        bvh->QueryOverlap(box, [&](const Triangle& tri) -> bool {
                            ++candidates;
                            const Vector3 cp = ClosestPointOnTriangle(p, tri);
                            const float d = Length(p - cp);
                            if (d < best) best = d;
                            return false; // 全候補を走査
                            });
                        if (candidates > 0) nearDist = best;
                    }

                    // 壁の近く（または接触中）なら毎フレーム、遠ければ0.25秒に1回
                    const bool nearGeometry = (nearDist >= 0.0f && nearDist <= radius + kProbeMargin);
                    static float s_lastStageLog = -1.0e9f;
                    const bool timeToLog = (nowSec_ - s_lastStageLog) >= 0.25f;
                    if (hit || nearGeometry || timeToLog) {
                        if (timeToLog) s_lastStageLog = nowSec_;

                        const AABB rb = bvh ? bvh->RootBounds() : AABB{};
                        std::string msg = "[StagePair] hit=" + std::to_string(hit ? 1 : 0)
                            + " depth=" + std::to_string(deepest.depth)
                            + " probe=(" + std::to_string(p.x) + "," + std::to_string(p.y) + "," + std::to_string(p.z) + ")"
                            + " r=" + std::to_string(radius)
                            + " triCount=" + std::to_string(bvh ? (int)bvh->TriangleCount() : -1)
                            + " cand=" + std::to_string(candidates)
                            + " nearDist=" + std::to_string(nearDist)
                            + " root=[(" + std::to_string(rb.min.x) + "," + std::to_string(rb.min.y) + "," + std::to_string(rb.min.z) + ")"
                            + "-(" + std::to_string(rb.max.x) + "," + std::to_string(rb.max.y) + "," + std::to_string(rb.max.z) + ")]\n";
                        OutputDebugStringA(msg.c_str());
                    }
                }
            }

            if (!hit) continue;

            // ---- 押し戻し（Blocking×Blocking・非ゲート＝毎フレーム）----
            // deepest.normal は c1 を c2 から押し出す向き。
            if (bothBlocking && haveContact && deepest.depth > 0.0f) {
                const Vector3 mtv = deepest.normal * deepest.depth;
                // [診断ログ] 押し戻しが発火したか。depth と法線を確認する。
                {
                    std::string msg = "[PushOut] id1=" + std::to_string((uint32_t)id1) + " id2=" + std::to_string((uint32_t)id2)
                        + " depth=" + std::to_string(deepest.depth)
                        + " n=(" + std::to_string(deepest.normal.x) + "," + std::to_string(deepest.normal.y) + "," + std::to_string(deepest.normal.z) + ")\n";
                    OutputDebugStringA(msg.c_str());
                }
                const bool m1 = c1->IsMovable();
                const bool m2 = c2->IsMovable();
                if (m1 && m2) {
                    c1->ApplyPushOut(mtv * 0.5f);
                    c2->ApplyPushOut(mtv * -0.5f);
                }
                else if (m1) {
                    c1->ApplyPushOut(mtv);
                }
                else if (m2) {
                    c2->ApplyPushOut(mtv * -1.0f);
                }
            }

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

	// ステージ(World)は「物理プロキシ(Object)」とだけ当てる。
	// 弾・武器・ヒットボディ等がステージ判定に巻き込まれないよう隔離する。
	if (g1 == CollisionGroup::World || g2 == CollisionGroup::World) {
		const bool worldVsBody =
			(g1 == CollisionGroup::World && g2 == CollisionGroup::Object) ||
			(g2 == CollisionGroup::World && g1 == CollisionGroup::Object);
		return !worldVsBody;
	}

	// 物理プロキシ(Object)はステージ(World)とだけ当てる（プロキシ同士は押し合わない）。
	if (g1 == CollisionGroup::Object || g2 == CollisionGroup::Object) {
		return true;
	}

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

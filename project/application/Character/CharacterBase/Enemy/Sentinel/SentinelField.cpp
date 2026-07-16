#include "SentinelField.h"
#include "Sentinel.h"
#include "CollisionManager.h"

SentinelField::SentinelField() = default;
SentinelField::~SentinelField() = default;

void SentinelField::Initialize(BaseScene* scene, const Vector3& center, float halfExtent)
{
    sentinels_.clear();

    // 正方形の四隅（XZ平面）。中央にはボスが立つ想定。
    const Vector3 corners[4] = {
        { center.x + halfExtent, center.y, center.z + halfExtent },
        { center.x + halfExtent, center.y, center.z - halfExtent },
        { center.x - halfExtent, center.y, center.z + halfExtent },
        { center.x - halfExtent, center.y, center.z - halfExtent },
    };

    for (const Vector3& pos : corners) {
        auto s = std::make_unique<Sentinel>(scene);
        s->InitializeSentinel(pos);
        sentinels_.push_back(std::move(s));
    }
}

void SentinelField::SetCamera(Camera* camera)
{
    for (auto& s : sentinels_) s->SetCamera(camera);
}

void SentinelField::SetDamagePopupSink(IDamagePopupSink* sink)
{
    for (auto& s : sentinels_) s->SetDamagePopupSink(sink);
}

void SentinelField::SetHiddenUnderground()
{
    for (auto& s : sentinels_) s->SetHiddenUnderground();
}

void SentinelField::StartSpawn()
{
    for (auto& s : sentinels_) s->StartSpawn();
}

void SentinelField::Update()
{
    for (auto& s : sentinels_) s->Update();
}

void SentinelField::Draw()
{
    for (auto& s : sentinels_) s->Draw();
}

void SentinelField::ParticleDraw()
{
    for (auto& s : sentinels_) s->ParticleDraw();
}

void SentinelField::ForeGroundDraw()
{
    for (auto& s : sentinels_) s->ForeGroundDraw();
}

void SentinelField::RegisterColliders(CollisionManager* cm)
{
    for (auto& s : sentinels_) {
        if (s->IsDead() || s->IsHidden()) continue; // 地中待機中は判定を出さない
        // 押し戻し用BVH（ステージと同じ枠）
        if (auto* bvh = s->GetBvhCollider()) {
            cm->RegisterCollider(bvh);
        }
        // 被弾スフィア
        cm->RegisterCollider(s->GetMultiCollider());
    }
}

void SentinelField::CollectAliveTargets(std::vector<ITarget*>& out)
{
    for (auto& s : sentinels_) {
        if (s->IsAlive()) out.push_back(s.get());
    }
}

int SentinelField::AliveCount() const
{
    int count = 0;
    for (const auto& s : sentinels_) {
        if (!s->IsDead()) ++count;
    }
    return count;
}

bool SentinelField::AllDefeated() const
{
    for (const auto& s : sentinels_) {
        if (!s->IsDead()) return false;
    }
    return !sentinels_.empty();
}

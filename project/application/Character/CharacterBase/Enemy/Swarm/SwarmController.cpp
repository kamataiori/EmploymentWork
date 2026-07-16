#include "SwarmController.h"
#include "SwarmEnemy.h"
#include "CollisionManager.h"
#include "engine/TimeManager.h"
#include <cmath>

SwarmController::SwarmController() = default;
SwarmController::~SwarmController() = default;

void SwarmController::Initialize(BaseScene* scene, const Transform* playerTarget)
{
    scene_ = scene;
    playerTarget_ = playerTarget;
    waveIndex_ = -1;
    cleared_ = false;
    enemies_.clear();
}

void SwarmController::SetCamera(Camera* camera)
{
    camera_ = camera;
    for (auto& e : enemies_) e->SetCamera(camera);
}

void SwarmController::SetDamagePopupSink(IDamagePopupSink* sink)
{
    sink_ = sink;
    for (auto& e : enemies_) e->SetDamagePopupSink(sink);
}

void SwarmController::Begin()
{
    if (waveIndex_ >= 0) return; // 二重開始を防ぐ
    SpawnWave(0);
}

void SwarmController::SpawnWave(int waveIndex)
{
    waveIndex_ = waveIndex;
    waveGapTimer_ = 0.0f;
    enemies_.clear();

    constexpr float kPi = 3.1415926535f;
    // 波ごとに少し角度をずらして、出てくる向きを変える
    const float baseAngle = static_cast<float>(waveIndex) * (kPi / static_cast<float>(kPerWave));

    for (int i = 0; i < kPerWave; ++i) {
        const float angle = baseAngle + (2.0f * kPi / static_cast<float>(kPerWave)) * static_cast<float>(i);
        const Vector3 pos{ std::sin(angle) * kRingRadius, 0.0f, std::cos(angle) * kRingRadius };

        auto e = std::make_unique<SwarmEnemy>(scene_);
        e->InitializeSwarm(pos);
        e->SetPlayerTarget(playerTarget_);
        if (camera_) e->SetCamera(camera_);
        if (sink_)   e->SetDamagePopupSink(sink_);
        enemies_.push_back(std::move(e));
    }
}

bool SwarmController::AllCurrentDead() const
{
    for (const auto& e : enemies_) {
        if (!e->IsDead()) return false;
    }
    return !enemies_.empty();
}

void SwarmController::Update()
{
    for (auto& e : enemies_) e->Update();

    if (cleared_) return;

    // 現在の波を殲滅したら、間を置いて次の波（最終波なら完了）
    if (AllCurrentDead()) {
        if (waveIndex_ + 1 < kWaveCount) {
            waveGapTimer_ += TimeManager::GetInstance()->GetDeltaTime();
            if (waveGapTimer_ >= kWaveGap) {
                SpawnWave(waveIndex_ + 1);
            }
        }
        else {
            cleared_ = true;
        }
    }
}

void SwarmController::Draw()
{
    for (auto& e : enemies_) e->Draw();
}

void SwarmController::ParticleDraw()
{
    for (auto& e : enemies_) e->ParticleDraw();
}

void SwarmController::ForeGroundDraw()
{
    for (auto& e : enemies_) e->ForeGroundDraw();
}

void SwarmController::RegisterColliders(CollisionManager* cm)
{
    for (auto& e : enemies_) {
        if (e->IsDead()) continue;
        cm->RegisterCollider(e->GetMultiCollider());
    }
}

void SwarmController::CollectAliveTargets(std::vector<ITarget*>& out)
{
    for (auto& e : enemies_) {
        if (e->IsAlive()) out.push_back(e.get());
    }
}

int SwarmController::AliveCount() const
{
    int count = 0;
    for (const auto& e : enemies_) {
        if (!e->IsDead()) ++count;
    }
    return count;
}

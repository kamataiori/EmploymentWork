#include "SwarmController.h"
#include "SwarmEnemy.h"
#include "CollisionManager.h"
#include "engine/TimeManager.h"
#include <cmath>

SwarmController::SwarmController() = default;
SwarmController::~SwarmController() = default;

void SwarmController::Initialize(BaseScene* scene, const Transform* playerTarget,
                                 const Vector3& ringCenter, float ringRadius)
{
    scene_ = scene;
    playerTarget_ = playerTarget;
    ringCenter_ = ringCenter;
    ringRadius_ = ringRadius;
    waveIndex_ = -1;
    cleared_ = false;

    // 全波分の個体をここで作り切っておく。生成にはシェーダのコンパイルや
    // パーティクルプリセットの読み込みが伴い1体でも数十msかかるため、
    // 波が来るたびに作るとその場でフレームが止まる（＝画面がガクつく）。
    // 波の切り替えでは Respawn で場に出すだけにする。
    enemies_.clear();
    enemies_.reserve(kWaveCount * kPerWave);
    for (int i = 0; i < kWaveCount * kPerWave; ++i) {
        auto e = std::make_unique<SwarmEnemy>(scene_);
        e->InitializeSwarm();
        e->SetPlayerTarget(playerTarget_);
        enemies_.push_back(std::move(e));
    }
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

    constexpr float kPi = 3.1415926535f;
    // 波ごとに少し角度をずらして、出てくる向きを変える
    const float baseAngle = static_cast<float>(waveIndex) * (kPi / static_cast<float>(kPerWave));

    // この波に割り当てられた個体を、外周の円上に置いて出す
    for (int i = 0; i < kPerWave; ++i) {
        const float angle = baseAngle + (2.0f * kPi / static_cast<float>(kPerWave)) * static_cast<float>(i);
        const Vector3 pos{
            ringCenter_.x + std::sin(angle) * ringRadius_,
            ringCenter_.y,
            ringCenter_.z + std::cos(angle) * ringRadius_,
        };
        enemies_[WaveSlotBegin(waveIndex) + i]->Respawn(pos);
    }
}

bool SwarmController::AllCurrentDead() const
{
    // 判定は現在の波の個体だけを見る（前の波の死体は含めない）
    for (int i = 0; i < kPerWave; ++i) {
        if (!enemies_[WaveSlotBegin(waveIndex_) + i]->IsDead()) return false;
    }
    return true;
}

void SwarmController::Update()
{
    for (auto& e : enemies_) {
        if (e->IsActive()) e->Update();
    }

    if (cleared_ || waveIndex_ < 0) return;

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
        if (!e->IsActive() || e->IsDead()) continue;
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
        if (e->IsActive() && !e->IsDead()) ++count;
    }
    return count;
}

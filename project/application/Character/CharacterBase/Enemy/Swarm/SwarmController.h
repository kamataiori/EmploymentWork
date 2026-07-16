#pragma once
#include "Vector3.h"
#include <memory>
#include <vector>

class BaseScene;
class Camera;
class CollisionManager;
class IDamagePopupSink;
class ITarget;
class SwarmEnemy;
struct Transform;

//======================================================
// SwarmController（第1波の群れを波状に出す管理役）
//------------------------------------------------------
// 1波あたり数体をステージ外周からスポーンさせ、全滅したら次の波、
// 最終波を殲滅したら AllWavesCleared() が true になる。
//======================================================
class SwarmController
{
public:
    SwarmController();
    ~SwarmController();

    // 追尾対象（プレイヤー）の Transform を渡して初期化する
    void Initialize(BaseScene* scene, const Transform* playerTarget);

    void SetCamera(Camera* camera);
    void SetDamagePopupSink(IDamagePopupSink* sink);

    // 第1波を出して波の進行を開始する（SwarmWaveState の入りで呼ぶ）
    void Begin();

    void Update();
    void Draw();
    void ParticleDraw();
    void ForeGroundDraw();

    void RegisterColliders(CollisionManager* cm);
    void CollectAliveTargets(std::vector<ITarget*>& out);

    int  AliveCount() const;
    bool AllWavesCleared() const { return cleared_; }

private:
    void SpawnWave(int waveIndex);
    bool AllCurrentDead() const;

    BaseScene*        scene_ = nullptr;
    const Transform*  playerTarget_ = nullptr;
    Camera*           camera_ = nullptr;
    IDamagePopupSink* sink_ = nullptr;

    std::vector<std::unique_ptr<SwarmEnemy>> enemies_; // 現在の波の個体

    int   waveIndex_ = -1;      // 現在の波（-1=未開始）
    float waveGapTimer_ = 0.0f; // 波と波の間の待ち
    bool  cleared_ = false;     // 全波を殲滅した

    // 配置・波の構成
    static constexpr int   kWaveCount   = 3;     // 波の数
    static constexpr int   kPerWave     = 3;     // 1波あたりの数
    static constexpr float kRingRadius  = 26.0f; // 外周スポーン半径（原点中心）
    static constexpr float kWaveGap     = 0.6f;  // 次の波までの間（秒）
};

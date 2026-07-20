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

    // 追尾対象（プレイヤー）の Transform と、スポーン外周（中心・半径）を渡して初期化する
    void Initialize(BaseScene* scene, const Transform* playerTarget,
                    const Vector3& ringCenter, float ringRadius);

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

    // enemies_ の中でその波が使う先頭の位置（各波は kPerWave 体ずつ固定で持つ）
    static constexpr int WaveSlotBegin(int waveIndex) { return waveIndex * kPerWave; }

    BaseScene*        scene_ = nullptr;
    const Transform*  playerTarget_ = nullptr;
    Camera*           camera_ = nullptr;
    IDamagePopupSink* sink_ = nullptr;

    // 全波分の個体を Initialize で作り切って持つ（波ごとに kPerWave 体ずつ割り当て）。
    // 出番が来た個体だけ Respawn で場に出す。実行中に生成しないための作りおき。
    std::vector<std::unique_ptr<SwarmEnemy>> enemies_;

    int   waveIndex_ = -1;      // 現在の波（-1=未開始）
    float waveGapTimer_ = 0.0f; // 波と波の間の待ち
    bool  cleared_ = false;     // 全波を殲滅した

    Vector3 ringCenter_{ 0.0f, 0.0f, 0.0f }; // スポーン外周の中心（手前の円エリア）
    float   ringRadius_ = 12.0f;             // スポーン外周の半径

    // 波の構成
    static constexpr int   kWaveCount = 3;    // 波の数
    static constexpr int   kPerWave   = 2;    // 1波あたりの数（2体×3波＝計6体）
    static constexpr float kWaveGap   = 0.6f; // 次の波までの間（秒）
};

#pragma once
#include <string>
#include <vector>
#include <Transform.h>
#include "ParticleModule.h"

// ===============================================
// エミッターのプリセット（静的データ）
// 今 ParticleManager が JSON に保存している
// 「SpawnSettings / UpdateSettings / RenderSettings」
// をこのクラスに移動させる
// 実行時にはコピーされず、EmitterInstance が参照する
// ===============================================

// ---------- Spawn ----------
struct SpawnSettings {
    uint32_t count = 1;      // Emit 時に生成される粒子数
    float frequency = 0.1f;  // 連続発生するときの間隔
    bool repeat = true;      // ループ発生するか
    bool randomOffset = false; // 位置にランダム性を持たせる
};

// ---------- Update ----------
struct UpdateSettings {
    Vector3 velocity{};      // 初速
    Vector3 rotationSpeed{}; // 回転速度
    Vector3 scaleSpeed{};    // スケールの増減
    float lifeTime = 1.0f;   // 粒子の寿命
    bool useGravity = false; // 重力を使うか
};

// ---------- Render ----------
struct RenderSettings {
    Vector4 color{ 1,1,1,1 };    // 色
    bool billboard = true;     // ビルボード
    bool flipY = false;        // 上下反転
    std::string textureFilePath; // 使用するテクスチャ
};

class ParticleEmitter {
public:
    std::string name;  // プリセット名（JSONのファイル名にもなる）

    SpawnSettings  spawn;
    UpdateSettings update;
    RenderSettings render;

    // ここに「ColorOverLifeModule」などを入れる
    std::vector<std::unique_ptr<ParticleModule>> modules;

public:
    // JSON の読み込み＆保存（ステップ1では空 or 既存コードを移植）
    void LoadFromJson(const std::string& filePath);
    void SaveToJson(const std::string& filePath);
};

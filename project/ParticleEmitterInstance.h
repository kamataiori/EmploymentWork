#pragma once
#include <vector>
#include <memory>
#include <string>

#include "MathFunctions.h"
#include "Particle.h"
#include "ParticleModule.h"

// ===============================================
// ParticleEmitterInstance
// 1つの「実行中エミッター」を表すクラス
// ・Transform（位置/回転/スケール）
// ・Particle 配列
// ・Spawn / Update / Render のランタイム値
// ===============================================
class ParticleEmitterInstance
{
public:
    ParticleEmitterInstance() = default;

    // ParticleManager::ParticlePreset* 相当のポインタを受け取る。
    // 型は void* にしておき、cpp 側で正しい型にキャストする。
    void Initialize(const void* presetRef);

    // Transform は外から直接触れるように public にしておく
    // （ParticleManager::CreateEmitterInstanceFromPreset で直接書き込んでいる）
    Transform transform_;

    // Emit 開始（repeat が true なら自動で繰り返す）
    void Emit();

    // その場で 1 回だけ Emit する
    void EmitOnce();

    // 毎フレーム更新
    void Update(float dt);

    // 描画処理（後でインスタンシング書き込みを移植）
    void Draw();

private:
    // 元プリセット（Curve など参照用）。型は void* で保持。
    const void* preset_ = nullptr;

    // 現在生きているパーティクル
    std::vector<Particle> particles_;

    // 追加モジュール（将来用）
    std::vector<std::unique_ptr<ParticleModule>> modules_;

    // 自分が基にしているプリセット名（EnsureGroupForPreset などで使う用）
    std::string presetName_;

    // Spawn 用
    uint32_t spawnCount_ = 1;     // 1回のEmitで出す数
    float    spawnInterval_ = 0.0f;  // 連続発生間隔(秒)
    bool     spawnRepeat_ = false; // ループさせるか
    float    spawnTimer_ = 0.0f;  // 間隔用タイマー
    bool     emitting_ = false; // Emit() 中かどうか

    // Update / Render 用の基本設定（Presetからコピーするイメージ）
    Vector3 baseVelocity_{};
    Vector3 baseRotationSpeed_{};
    Vector3 baseScaleSpeed_{};
    float   baseLifeTime_ = 1.0f;
    bool    useGravity_ = false;

    Vector4 baseColor_{ 1,1,1,1 };
    bool    billboard_ = true;
    bool    flipY_ = false;
};

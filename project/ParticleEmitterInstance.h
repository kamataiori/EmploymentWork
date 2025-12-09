#pragma once
#include <vector>
#include <memory>
#include <string>

#include "MathFunctions.h"
#include "Particle.h"
#include "ParticleModule.h"
#include "ParticlePreset.h"

// Emitter のインスタンス（実行中の1個のエミッタ）
class ParticleEmitterInstance
{
public:
    ParticleEmitterInstance() = default;

    // ParticleManager::ParticlePreset* をそのまま渡してOK
    // （ここでは const void* として受け取り、cpp 側でキャスト）
    void Initialize(const void* presetRef);

    // Emit 開始（repeat が true なら自動で繰り返す）
    void Emit();

    // その場で 1 回だけ Emit する
    void EmitOnce();

    // 毎フレーム更新
    void Update(float dt);

    // 描画処理
    // ※ 実際の DrawInstanced は ParticleManager::Draw() 側でやるので、
    //   ここでは「必要なら将来用の処理」を書く程度。現状は何もしなくてOK。
    void Draw();

    // 外から Transform を直接いじる用
    Transform& GetTransform() { return transform_; }
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& t) { transform_ = t; }

    // ==== ParticleManager から読むためのゲッター ====

    // 今このエミッタが持っているパーティクル一覧
    const std::vector<Particle>& GetParticles() const { return particles_; }

    // 自分が参照しているプリセット（ParticleManager::ParticlePreset）への生ポインタ
    const void* GetPresetRaw() const { return preset_; }

    // プリセット名（=グループ名）を覚えておく
    const std::string& GetPresetName() const { return presetName_; }

    bool IsBillboard() const { return billboard_; }
    bool IsFlipY() const { return flipY_; }

private:
    // 生成元プリセットへのポインタ（実際の型は ParticleManager::ParticlePreset）
    const void* preset_ = nullptr;

    // プリセット名（= groupName 用）
    std::string presetName_;

    // 実行中パーティクル
    std::vector<Particle> particles_;

    // Transform（エミッタの位置・回転・スケール）
    Transform transform_{};

    // Spawn 用
    uint32_t spawnCount_ = 1;
    float    spawnInterval_ = 0.0f;
    bool     spawnRepeat_ = false;
    float    spawnTimer_ = 0.0f;
    bool     emitting_ = false;

    // Update / Render の基本値（プリセットからコピーしておく）
    Vector3 baseVelocity_{};
    Vector3 baseRotationSpeed_{};
    Vector3 baseScaleSpeed_{};
    float   baseLifeTime_ = 1.0f;
    bool    useGravity_ = false;

    Vector4 baseColor_{ 1,1,1,1 };
    bool    billboard_ = true;
    bool    flipY_ = false;

    // 将来的な Module 用
    std::vector<std::unique_ptr<ParticleModule>> modules_;
};

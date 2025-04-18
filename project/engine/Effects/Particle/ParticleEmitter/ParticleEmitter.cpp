#include "ParticleEmitter.h"

ParticleEmitter::ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform, uint32_t count, float frequency, bool repeat)
    : particleManager_(particleManager), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat)
{
    Emit(); // 初期化時に即時発生
}

// コンストラクタ
ParticleEmitter::ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform, uint32_t count, float frequency, bool repeat, bool usePrimitive)
    : particleManager_(particleManager), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat), usePrimitive_(usePrimitive)
{
    PrimitiveEmit();
}

ParticleEmitter::ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform)
    : particleManager_(particleManager), name_(name), transform_(transform), useRing_(true)
{
    RingEmit();
}

ParticleEmitter::ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform, bool useCylinder)
    : particleManager_(particleManager), name_(name), transform_(transform), useCylinder_(useCylinder)
{
    CylinderEmit();
}

// 更新処理
void ParticleEmitter::Update()
{
    if (!repeat_) return; // 繰り返しフラグがfalseの場合は処理をスキップ

    elapsedTime_ += 1.0f / 60.0f; // フレーム単位の経過時間を加算

    if (elapsedTime_ >= frequency_)
    {
        if (usePrimitive_) {
            PrimitiveEmit();
        }
        else {
            Emit();
        }
        elapsedTime_ -= frequency_; // 周期的に実行するためリセット
    }
}

// Emit処理
void ParticleEmitter::Emit()
{
    particleManager_->Emit(name_, transform_.translate, count_);
}

void ParticleEmitter::PrimitiveEmit()
{
    particleManager_->PrimitiveEmit(name_, transform_.translate, count_);
}

void ParticleEmitter::RingEmit()
{
    particleManager_->RingEmit(name_, transform_.translate);
}

void ParticleEmitter::CylinderEmit()
{
    particleManager_->CylinderEmit(name_, transform_.translate);
}

// 繰り返し設定
void ParticleEmitter::SetRepeat(bool repeat)
{
    repeat_ = repeat;
}

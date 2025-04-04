#include "ParticleEmitter.h"

// コンストラクタ
ParticleEmitter::ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform, uint32_t count, float frequency, bool repeat)
    : particleManager_(particleManager), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat)
{
    Emit(); // 初期化時に即時発生
}

// 更新処理
void ParticleEmitter::Update()
{
    if (!repeat_) return; // 繰り返しフラグがfalseの場合は処理をスキップ

    elapsedTime_ += 1.0f / 60.0f; // フレーム単位の経過時間を加算

    if (elapsedTime_ >= frequency_)
    {
        Emit();
        elapsedTime_ -= frequency_; // 周期的に実行するためリセット
    }
}

// Emit処理
void ParticleEmitter::Emit()
{
    particleManager_->Emit(name_, transform_.translate, count_);
}

// 繰り返し設定
void ParticleEmitter::SetRepeat(bool repeat)
{
    repeat_ = repeat;
}

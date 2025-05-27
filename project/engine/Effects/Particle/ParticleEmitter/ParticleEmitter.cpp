#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include <cassert>

/// 初期化関数（エミッターの設定と初回Emitを行う）
void ParticleEmitter::Initialize(ParticleManager* particleManager, const std::string& name, const Transform& transform, const EmitterConfig& config)
{
    assert(particleManager); // nullチェック

    particleManager_ = particleManager;
    name_ = name;
    transform_ = transform;
    config_ = config;
    elapsedTime_ = config.frequency; // 最初のEmitが即時起きるように設定
    isInitialized_ = true;

    EmitByShape(); // 初期発生
}

/// 更新処理（repeatが有効なら一定時間ごとにEmit）
void ParticleEmitter::Update()
{
    if (!isInitialized_ || !config_.repeat) return;

    // フレーム経過（固定60FPS想定）
    elapsedTime_ += 1.0f / 60.0f;

    // 指定の間隔を超えたらEmit実行
    if (elapsedTime_ >= config_.frequency) {
        EmitByShape();
        elapsedTime_ -= config_.frequency;
    }
}

/// 外部からEmitを強制的に発生させる
void ParticleEmitter::Emit()
{
    if (!isInitialized_) return;
    EmitByShape();
}

/// パーティクルの形状に応じたEmitを行う
void ParticleEmitter::EmitByShape()
{
    switch (config_.shapeType) {
    case ShapeType::Plane:
        particleManager_->Emit(name_, transform_.translate, config_.count);
        break;
    case ShapeType::Primitive:
        particleManager_->PrimitiveEmit(name_, transform_.translate, config_.count);
        break;
    case ShapeType::Ring:
        particleManager_->RingEmit(name_, transform_.translate);
        break;
    case ShapeType::Cylinder:
        particleManager_->CylinderEmit(name_, transform_.translate);
        break;
    default:
        break;
    }
}

// ----- Setter -----

void ParticleEmitter::SetRepeat(bool repeat) {
    config_.repeat = repeat;
}

void ParticleEmitter::SetCount(uint32_t count) {
    config_.count = count;
}

void ParticleEmitter::SetFrequency(float frequency) {
    config_.frequency = frequency;
}

void ParticleEmitter::SetTransform(const Transform& transform) {
    transform_ = transform;
}

void ParticleEmitter::SetPosition(const Vector3& position) {
    transform_.translate = position;
}

void ParticleEmitter::SetRotation(const Vector3& rotation) {
    transform_.rotate = rotation;
}

void ParticleEmitter::SetScale(const Vector3& scale) {
    transform_.scale = scale;
}


// ----- Getter -----

const Transform& ParticleEmitter::GetTransform() const {
    return transform_;
}

Transform& ParticleEmitter::GetTransform() {
    return transform_;
}

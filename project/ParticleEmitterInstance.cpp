#include "ParticleEmitterInstance.h"
#include "ParticleManager.h"   // ParticleManager::ParticlePreset を見るため
#include <algorithm>

// 重力加速度（ParticleManager 側の挙動に合わせる）
static constexpr float kGravity = -9.81f;

// 便利用：内部で使うエイリアス
using PMPreset = ParticleManager::ParticlePreset;

void ParticleEmitterInstance::Initialize(const void* presetRef)
{
    preset_ = presetRef;

    const auto* preset = reinterpret_cast<const PMPreset*>(presetRef);
    if (!preset) {
        return;
    }

    // プリセット名を覚えておく（= ParticleGroup のキーになる）
    presetName_ = preset->name;

    // ==== Spawn 系 ====
    spawnCount_ = preset->emitterSpawn.count;
    spawnInterval_ = preset->emitterSpawn.frequency;
    spawnRepeat_ = preset->emitterSpawn.repeat;
    spawnTimer_ = 0.0f;
    emitting_ = false;

    // ==== Update 系 ====
    baseLifeTime_ = preset->particleUpdate.lifeTime;
    baseVelocity_ = preset->particleUpdate.velocity;
    baseRotationSpeed_ = preset->particleUpdate.rotationSpeed;
    baseScaleSpeed_ = preset->particleUpdate.scaleSpeed;
    useGravity_ = preset->particleUpdate.useGravity;

    // ==== Render 系 ====
    baseColor_ = preset->render.color;
    billboard_ = preset->render.useBillboard;
    flipY_ = preset->render.flipY;

    // ざっくり確保（あとで必要なら調整）
    if (spawnCount_ > 0) {
        particles_.reserve(spawnCount_ * 4);
    }
}

void ParticleEmitterInstance::Emit()
{
    if (!preset_) {
        return;
    }

    emitting_ = true;
    spawnTimer_ = 0.0f;

    // 1回目は即時発生
    EmitOnce();

    // ループしない設定ならここで止める
    if (!spawnRepeat_) {
        emitting_ = false;
    }
}

void ParticleEmitterInstance::EmitOnce()
{
    if (!preset_ || spawnCount_ == 0) {
        return;
    }

    for (uint32_t i = 0; i < spawnCount_; ++i) {
        Particle p{};

        p.active = true;
        p.life = 0.0f;
        p.maxLife = baseLifeTime_;

        // エミッタの Transform から初期Transformを設定
        p.position = transform_.translate;
        p.rotation = transform_.rotate;
        p.scale = transform_.scale;
        p.initialScale = transform_.scale;

        p.velocity = baseVelocity_;
        p.rotationSpeed = baseRotationSpeed_;
        p.scaleSpeed = baseScaleSpeed_;

        p.color = baseColor_;
        p.initialColor = baseColor_;

        // inactive スロットがあれば再利用
        bool reused = false;
        for (auto& existing : particles_) {
            if (!existing.active) {
                existing = p;
                reused = true;
                break;
            }
        }
        if (!reused) {
            particles_.push_back(p);
        }

        // Module 側でSpawn時にいじりたい場合用
        for (auto& m : modules_) {
            m->ApplySpawn(*this, p);
        }
    }
}

void ParticleEmitterInstance::Update(float dt)
{
    if (!preset_) {
        return;
    }

    const auto* preset = reinterpret_cast<const PMPreset*>(preset_);

    // ==== 自動発生（repeat=true のとき） ====
    if (spawnRepeat_ && emitting_) {
        spawnTimer_ -= dt;
        while (spawnTimer_ <= 0.0f) {
            EmitOnce();
            spawnTimer_ += spawnInterval_;

            if (spawnInterval_ <= 0.0f) {
                emitting_ = false;
                break;
            }
        }
    }

    // ==== 各パーティクル更新 ====
    for (auto& p : particles_) {
        if (!p.active) {
            continue;
        }

        p.life += dt;
        if (p.life >= p.maxLife) {
            p.active = false;
            continue;
        }

        float age = (p.maxLife > 0.0f) ? (p.life / p.maxLife) : 0.0f;
        age = std::clamp(age, 0.0f, 1.0f);

        // 重力
        if (useGravity_) {
            p.velocity.y += kGravity * dt;
        }

        // 位置
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;

        // 回転
        p.rotation.x += p.rotationSpeed.x * dt;
        p.rotation.y += p.rotationSpeed.y * dt;
        p.rotation.z += p.rotationSpeed.z * dt;

        // スケール：カーブ優先
        const auto& scaleCurve = preset->particleUpdate.scaleCurve;
        if (scaleCurve.enabled && !scaleCurve.keys.empty()) {
            float s = scaleCurve.Evaluate(age);
            p.scale.x = p.initialScale.x * s;
            p.scale.y = p.initialScale.y * s;
            p.scale.z = p.initialScale.z * s;
        }
        else {
            p.scale.x += p.scaleSpeed.x * dt;
            p.scale.y += p.scaleSpeed.y * dt;
            p.scale.z += p.scaleSpeed.z * dt;
        }

        // 色：colorCurve を適用
        const auto& colorCurve = preset->render.colorCurve;
        if (colorCurve.enabled && !colorCurve.keys.empty()) {
            float c = colorCurve.Evaluate(age);
            p.color.x = p.initialColor.x * c;
            p.color.y = p.initialColor.y * c;
            p.color.z = p.initialColor.z * c;
            p.color.w = p.initialColor.w * c;
        }

        // Module による追加更新
        for (auto& m : modules_) {
            m->ApplyUpdate(*this, p, dt);
        }
    }

    // ※ 死んだParticleを vector から消すのはコストも大きいので、
    //   今は active フラグだけで管理しています。
}

void ParticleEmitterInstance::Draw()
{
    // 現状は何もしない。
    // 実際の描画は ParticleManager::Update() が instancingDataPtr に
    // particles_ の内容を書き込んで行う。
}

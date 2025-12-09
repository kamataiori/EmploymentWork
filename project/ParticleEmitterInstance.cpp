#include "ParticleEmitterInstance.h"
#include "ParticleManager.h"
#include <algorithm>

// 重力加速度（ParticleManager 側の挙動に合わせる）
static constexpr float kGravity = -9.81f;

void ParticleEmitterInstance::Initialize(const void* presetRef)
{
    preset_ = presetRef;

    // ParticleManager 側のプリセット型にキャスト
    const auto* preset = reinterpret_cast<const ParticleManager::ParticlePreset*>(presetRef);
    if (!preset) {
        return;
    }

    // ここでプリセット名を覚えておく
    presetName_ = preset->name;

    // ---- Spawn 系 ----
    spawnCount_ = preset->emitterSpawn.count;
    spawnInterval_ = preset->emitterSpawn.frequency;
    spawnRepeat_ = preset->emitterSpawn.repeat;
    spawnTimer_ = 0.0f;
    emitting_ = false;

    // ---- Update 系 ----
    baseLifeTime_ = preset->particleUpdate.lifeTime;
    baseVelocity_ = preset->particleUpdate.velocity;
    baseRotationSpeed_ = preset->particleUpdate.rotationSpeed;
    baseScaleSpeed_ = preset->particleUpdate.scaleSpeed;
    useGravity_ = preset->particleUpdate.useGravity;

    // ---- Render 系 ----
    baseColor_ = preset->render.color;
    billboard_ = preset->render.useBillboard;
    flipY_ = preset->render.flipY;

    // 必要に応じて粒子配列をあらかじめ確保
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
    spawnTimer_ = 0.0f; // 即時に 1 回目を出す

    EmitOnce();

    // 繰り返し設定でない場合は、1 回出したら終わり
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

        // 位置・回転はエミッターの Transform から
        p.position = transform_.translate;
        p.rotation = transform_.rotate;

        // スケールは Transform のスケールを初期値として保持
        p.scale = transform_.scale;
        p.initialScale = transform_.scale;

        // 速度 / 回転速度 / スケール速度
        p.velocity = baseVelocity_;
        p.rotationSpeed = baseRotationSpeed_;
        p.scaleSpeed = baseScaleSpeed_;

        // 色も「現在値」と「初期値」の両方に入れておく
        p.color = baseColor_;
        p.initialColor = baseColor_;

        // 既存スロットの再利用（inactive なものがあればそこを上書き）
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

        // モジュール側にも初期化フックを渡す（将来用）
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

    const auto* preset = reinterpret_cast<const ParticleManager::ParticlePreset*>(preset_);

    // ---- 自動発生（repeat=true のとき）----
    if (spawnRepeat_ && emitting_) {
        spawnTimer_ -= dt;
        while (spawnTimer_ <= 0.0f) {
            EmitOnce();
            spawnTimer_ += spawnInterval_;

            // spawnInterval_ <= 0 の場合は無限ループ防止で止める
            if (spawnInterval_ <= 0.0f) {
                emitting_ = false;
                break;
            }
        }
    }

    // ---- 各パーティクルの更新 ----
    for (auto& p : particles_) {
        if (!p.active) {
            continue;
        }

        // 寿命更新
        p.life += dt;
        if (p.life >= p.maxLife) {
            p.active = false;
            continue;
        }

        // NormalizedAge を 0～1 にクランプ
        float age = (p.maxLife > 0.0f) ? (p.life / p.maxLife) : 0.0f;
        age = std::clamp(age, 0.0f, 1.0f);

        // 重力
        if (useGravity_) {
            p.velocity.y += kGravity * dt;
        }

        // 位置更新
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;

        // 回転更新
        p.rotation.x += p.rotationSpeed.x * dt;
        p.rotation.y += p.rotationSpeed.y * dt;
        p.rotation.z += p.rotationSpeed.z * dt;

        // ---- スケール更新（カーブ or 速度）----
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

        // ---- 色更新（colorCurve）----
        const auto& colorCurve = preset->render.colorCurve;
        if (colorCurve.enabled && !colorCurve.keys.empty()) {
            float c = colorCurve.Evaluate(age);
            p.color.x = p.initialColor.x * c;
            p.color.y = p.initialColor.y * c;
            p.color.z = p.initialColor.z * c;
            p.color.w = p.initialColor.w * c;
        }

        // モジュールによる Update（将来用）
        for (auto& m : modules_) {
            m->ApplyUpdate(*this, p, dt);
        }
    }
}

void ParticleEmitterInstance::Draw()
{
    if (!preset_) {
        return;
    }

    // 現状の描画は ParticleManager::Draw() 内にあるので、
    // ここではまだ何もしない。
    //
    // 将来的には：
    //  - particles_ の内容からインスタンシング用頂点データを組み立てる
    //  - billboard_ や flipY_ を使ってワールド行列やUVを調整する
    //  - ParticleManager 側の VertexBuffer に書き込む
    // といった処理をこのクラスへ移していく予定。
}

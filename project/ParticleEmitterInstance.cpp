#include "ParticleEmitterInstance.h"
// ここで ParticlePreset の実体定義が必要になるので、
// 現状は ParticleManager.h を include しても OK。
// 将来、ParticlePreset を外出ししたら差し替える。
#include "ParticleManager.h"

void ParticleEmitterInstance::Initialize(ParticlePreset* presetRef)
{
    preset_ = presetRef;

    // 必要なら粒子配列の予約など
    // 例：preset の maxCount 相当の値があれば reserve する、など
    // particles_.reserve(256);
}

void ParticleEmitterInstance::Emit()
{
    if (!preset_) {
        return;
    }

    // ★ 今は簡易的なダミー実装：
    //   - 本格的な処理は後で ParticleManager から移植する
    //
    // ここで本来は：
    //   - preset_->emitterSpawn.count, frequency などを見て
    //   - 必要な数だけ Particle を生成し
    //   - preset_->particleSpawn / particleUpdate を使って初期化する
    //
    // 今はとりあえず「1個だけ、原点に寿命付きで作る」
    Particle p;
    p.active = true;
    p.position = transform_.translate; // Transform の位置
    p.scale = { 1, 1, 1 };
    p.color = { 1, 1, 1, 1 };
    p.life = 0.0f;
    p.maxLife = 1.0f;

    // モジュール側にも初期化フックを渡す（将来用）
    for (auto& m : modules_) {
        m->ApplySpawn(*this, p);
    }

    particles_.push_back(p);
}

void ParticleEmitterInstance::EmitOnce()
{
    for (uint32_t i = 0; i < spawnCount_; ++i) {
        Particle p{};
        p.active = true;
        p.position = transform_.translate;      // 位置
        p.velocity = baseVelocity_;            // 速度
        p.rotation = transform_.rotate;
        p.scale = transform_.scale;
        p.rotationSpeed = baseRotationSpeed_;
        p.scaleSpeed = baseScaleSpeed_;
        p.color = baseColor_;
        p.life = 0.0f;
        p.maxLife = baseLifeTime_;

        particles_.push_back(p);
    }
}

void ParticleEmitterInstance::Update(float dt)
{
    //if (!preset_) {
    //    return;
    //}

    //for (auto& p : particles_) {
    //    if (!p.active) { continue; }

    //    // 寿命更新
    //    p.life += dt;
    //    if (p.life >= p.maxLife) {
    //        p.active = false;
    //        continue;
    //    }

    //    // 簡易：速度だけ適用（本格版は particleUpdateModule を見る）
    //    p.position = Add(p.position, Multiply(dt, p.velocity));

    //    // モジュールで追加カスタム更新
    //    for (auto& m : modules_) {
    //        m->ApplyUpdate(*this, p, dt);
    //    }
    //}

    // 死んだパーティクルを削除してもいい（が、GCコストが気になるならフラグ方式でOK）
    // 今はそのままにしておき、後で最適化で詰める。


    // 1) ループ発生ならタイマーで EmitOnce を呼ぶ
    if (spawnRepeat_) {
        spawnTimer_ += dt;
        while (spawnTimer_ >= spawnInterval_) {
            spawnTimer_ -= spawnInterval_;
            EmitOnce();
        }
    }

    // 2) 既存粒子の更新
    for (auto& p : particles_) {
        if (!p.active) { continue; }

        p.life += dt;
        if (p.life >= p.maxLife) {
            p.active = false;
            continue;
        }

        // 速度・重力
        if (useGravity_) {
            p.velocity.y -= 0.98f * dt;  // 既存の重力処理と同じにする
        }
        p.position += p.velocity * dt;

        // 回転・スケール
        p.rotation += p.rotationSpeed * dt;
        p.scale += p.scaleSpeed * dt;

        // ColorOverLife / ScaleOverLife のカーブが必要なら、
        // ここで NormalizedAge = p.life / p.maxLife を使って評価する
}

void ParticleEmitterInstance::Draw()
{
    if (!preset_) {
        return;
    }

    // ★ 現状の描画は ParticleManager::Draw() 内にあるので、
    //   ここでは「将来ここに移す」前提で何もしない or
    //   必要な情報を外部へ渡す形だけ決めておく。
    //
    // たとえば：
    //   - VertexBuffer に書き込むデータを作る
    //   - カメラ情報を使って Billboard 行列を計算する
    //   - Texture / BlendMode を preset_ から引く
    //
    // 今は空実装で OK（移植フェーズで中身を入れていく）。
}

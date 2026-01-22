#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(const std::string& name)
    : name_(name)
{
    // Transform の初期値（必要なら調整）
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = { 0.0f, 0.0f, 0.0f };
}

ParticleEmitterInstance* ParticleSystem::AddEmitter(
    ParticleEmitterInstance* emitter,
    float startTime,
    bool autoPlay)
{
    if (!emitter) {
        return nullptr;
    }
    EmitterEntry entry;
    entry.emitter = emitter;
    entry.startTime = startTime;
    entry.autoPlay = autoPlay;
    entry.playedOnce = false;

    emitters_.push_back(entry);
    return emitter;
}

void ParticleSystem::AddPresetName(const std::string& presetName)
{
    if (presetName.empty()) {
        return;
    }

    // 重複チェック
    for (const auto& n : presetNames_) {
        if (n == presetName) {
            return;
        }
    }
    presetNames_.push_back(presetName);
}

// ====== 再生制御 ======

void ParticleSystem::Play()
{
    time_ = 0.0f;
    playing_ = true;

    // すべての Emitter を初期状態に戻してから、時間に応じて再生させる
    for (auto& e : emitters_) {
        e.playedOnce = false;
        if (e.emitter) {
            e.emitter->Reset();
            e.emitter->Stop(); // startTime に達するまでは止めておく
        }
    }
}

void ParticleSystem::Stop()
{
    playing_ = false;

    // 必要なら Emitter も止める
    for (auto& e : emitters_) {
        if (e.emitter) {
            e.emitter->Stop();
        }
    }
}

void ParticleSystem::Reset()
{
    time_ = 0.0f;

    for (auto& e : emitters_) {
        e.playedOnce = false;
        if (e.emitter) {
            e.emitter->Reset();
            e.emitter->Stop();
        }
    }
}

// ====== Update ======

void ParticleSystem::Update(float dt)
{
    if (!playing_) {
        return;
    }

    time_ += dt;

    // 再生開始時間に達した Emitter に Play をかけるだけ
    // ※ Emitter の Update(dt)（シミュレーション）は ParticleManager 側で行う
    for (auto& e : emitters_) {
        if (!e.emitter) {
            continue;
        }

        // すでに一度再生した Emitter はスキップ（シンプルな一回再生）
        if (e.playedOnce) {
            continue;
        }

        // startTime に達していなければまだ再生しない
        if (time_ < e.startTime) {
            continue;
        }

        if (e.autoPlay) {
            e.emitter->Reset();
            e.emitter->Play();
        }

        e.playedOnce = true;
    }

    // ===== ここから System 自体のループ処理 =====
    if (duration_ > 0.0f && time_ >= duration_) {
        if (loop_) {
            // 再生し直す。time_ や Emitter 状態を初期化
            Play();
        }
        else {
            // 1 回で終了
            playing_ = false;
        }
    }
}

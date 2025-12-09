#include "ParticleSystem.h"
#include "ParticleEmitterInstance.h"
#include <algorithm>

ParticleSystem::ParticleSystem(const std::string& name)
    : name_(name)
{
}

// 既存：AddEmitter / Update / Draw はそのままでOK

void ParticleSystem::AddEmitter(ParticleEmitterInstance* emitter)
{
    if (!emitter) { return; }
    emitters_.push_back(emitter);
}

void ParticleSystem::Update(float dt)
{
    for (auto* e : emitters_) {
        if (e) {
            e->Update(dt);
        }
    }
}

void ParticleSystem::Draw()
{
    for (auto* e : emitters_) {
        if (e) {
            e->Draw();
        }
    }
}

// System 定義としてのプリセット名リスト管理
void ParticleSystem::AddPresetName(const std::string& presetName)
{
    if (presetName.empty()) {
        return;
    }

    // 重複を避ける
    auto it = std::find(presetNames_.begin(), presetNames_.end(), presetName);
    if (it == presetNames_.end()) {
        presetNames_.push_back(presetName);
    }
}

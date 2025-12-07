#include "ParticleSystem.h"

ParticleEmitterInstance* ParticleSystem::AddEmitter(ParticleEmitterInstance* emitter)
{
    if (!emitter) {
        return nullptr;
    }

    emitters_.push_back(emitter);
    return emitter;
}

void ParticleSystem::Update(float dt)
{
    // 今はまだ ParticleManager 側で一括 Update しているので
    // ここは呼ばれていませんが、将来 System 単位で制御したくなった時に使えます。
    for (auto* emitter : emitters_) {
        if (emitter) {
            emitter->Update(dt);
        }
    }
}

void ParticleSystem::Draw()
{
    // ここも今は未使用。将来 System 単位描画に移行するとき用。
    for (auto* emitter : emitters_) {
        if (emitter) {
            emitter->Draw();
        }
    }
}

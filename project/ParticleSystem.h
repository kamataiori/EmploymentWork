// ParticleSystem.h
#pragma once
#include <string>
#include <vector>
#include <Transform.h>

class ParticleEmitterInstance;

class ParticleSystem {
public:
    ParticleSystem(const std::string& name);

    const std::string& GetName() const { return name_; }

    Transform& GetTransform() { return systemTransform_; }
    const Transform& GetTransform() const { return systemTransform_; }

    // ---- 既存 ----
    void AddEmitter(ParticleEmitterInstance* emitter);
    void Update(float dt);
    void Draw();

    // ---- 追加：System に紐づくプリセット定義 ----
    void AddPresetName(const std::string& presetName);
    const std::vector<std::string>& GetPresetNames() const { return presetNames_; }

private:
    std::string name_;
    Transform systemTransform_;

    // この System に属する EmitterInstance（ランタイム）
    std::vector<ParticleEmitterInstance*> emitters_;

    // この System で Emit したいプリセット名一覧（定義）
    std::vector<std::string> presetNames_;
};

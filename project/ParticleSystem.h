#pragma once
#include <string>
#include <vector>
#include <memory>
#include <Transform.h>
#include "ParticleEmitterInstance.h"

// ===============================================
// ParticleSystem
// 1つの「エフェクト」を表すクラス。
// - システム名（Editor でユーザーが付ける名前）
// - システム全体の Transform
// - 複数の ParticleEmitterInstance を「名前でまとめて」管理
//
// ここでは「Emitter の所有権」は持たず、
// ParticleManager 側の vector<unique_ptr<ParticleEmitterInstance>>
// に所有させたまま、System では「生ポインタだけを束ねる」形にします。
// ===============================================
class ParticleSystem
{
public:
    using EmitterList = std::vector<ParticleEmitterInstance*>;

private:
    // システム名（Niagara System 名みたいなもの）
    std::string name_;

    // システム全体の基準 Transform
    // （今はまだ使わないが、後で各 Emitter の相対 Transform に使える）
    Transform   transform_{};

    // この System にぶら下がっている Emitter の一覧
    // 所有権は持たず、生ポインタだけを保持する
    EmitterList emitters_;

public:
    ParticleSystem() = default;
    explicit ParticleSystem(const std::string& name)
        : name_(name)
    {
    }

    // ===== アクセサ =====
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& t) { transform_ = t; }

    const EmitterList& GetEmitters() const { return emitters_; }

    // ===== Emitter の追加 =====
    // ParticleManager 側で生成済みの EmitterInstance を登録するだけ
    ParticleEmitterInstance* AddEmitter(ParticleEmitterInstance* emitter);

    // 将来的に System 単位で Update / Draw したくなった時用の関数
    void Update(float dt);
    void Draw();
};

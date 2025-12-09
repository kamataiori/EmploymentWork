#pragma once
#include <string>
#include <vector>
#include "Transform.h"
#include "ParticleEmitterInstance.h"

// ===============================================
// ParticleSystem
// 1つの「エフェクト」を表すクラス。
// - System 名（Editor でユーザーが付ける名前）
// - System 全体の Transform
// - 複数の ParticleEmitterInstance を保持
// - 「この System に登録されているプリセット名一覧」を持つ
//
// ※ 所有権は ParticleManager 側の
//   std::vector<std::unique_ptr<ParticleEmitterInstance>>
//   にあります。ここは生ポインタで参照するだけ。
//
// ※ Draw は持たず、「再生タイミング制御（Play/Stop）」だけ行う。
//    実際のシミュレーション（Update(dt)）と描画は ParticleManager が担当。
// ===============================================
class ParticleSystem
{
public:
    struct EmitterEntry {
        ParticleEmitterInstance* emitter = nullptr;
        float startTime = 0.0f;   // System の time_ 基準でいつ再生開始するか
        bool  autoPlay = true;   // 再生開始時に自動で Play するか
        bool  playedOnce = false; // 一度 Play したかどうか
    };

    using EmitterList = std::vector<EmitterEntry>;

    explicit ParticleSystem(const std::string& name = "");

    // ----- System 名 -----
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& n) { name_ = n; }

    // ----- System 全体の Transform（必要なら使用） -----
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& t) { transform_ = t; }

    // ----- 所属エミッタ一覧 -----
    const EmitterList& GetEmitters() const { return emitters_; }

    // ParticleManager 側で生成済みの EmitterInstance を登録するだけ
    // startTime: 再生開始時間（System 再生開始からの秒数）
    // autoPlay : time_ >= startTime になったときに自動で Play するか
    ParticleEmitterInstance* AddEmitter(
        ParticleEmitterInstance* emitter,
        float startTime = 0.0f,
        bool autoPlay = true
    );

    // ----- System に紐付いているプリセット名一覧 -----
    const std::vector<std::string>& GetPresetNames() const { return presetNames_; }

    // 同じ名前があれば追加しない
    void AddPresetName(const std::string& presetName);
    void ClearPresetNames() { presetNames_.clear(); }

    // ====== 再生制御 ======
    void Play();   // time_ を 0 にして再生開始
    void Stop();   // System の時間を止め、必要なら Emitter も Stop
    void Reset();  // time_ を 0 に戻し、Emitter も Reset + Stop
    bool IsPlaying() const { return playing_; }

    // ====== 毎フレーム更新 ======
    // ・System の time_ を進める
    // ・startTime に達した Emitter に対して Play() を呼ぶ など
    // ※ Emitter の Update(dt) はここでは呼ばない
    void Update(float dt);

private:
    std::string name_;
    Transform   transform_{};

    // 実行時にぶら下がるエミッタ
    EmitterList emitters_;

    // 「この System で使うプリセット名」の一覧
    std::vector<std::string> presetNames_;

    // System 内部時間
    float time_ = 0.0f;
    bool  playing_ = false;
};

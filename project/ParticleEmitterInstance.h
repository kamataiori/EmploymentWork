#pragma once
#include <vector>
#include <memory>
#include <string>

#include "MathFunctions.h"
#include "Particle.h"
#include "ParticleModule.h"


// 前方宣言：
// ここでは「プリセットの型」はまだ ParticleManager 側にある前提
// 後で Step2 以降で独立させる
class ParticlePreset;

//
// ===============================================
// ParticleEmitterInstance
// 「実際に動いているエミッター1個」を表すクラス。
// - プリセット（ParticlePreset）への参照
// - Transform（位置/回転/スケール）
// - Particle 配列
// - 必要なら追加のモジュール（ParticleModule）
// 
// この段階では、まだ ParticleManager と接続しない。
// 先にクラスだけ用意して、あとから徐々にロジックを移植する。
// ===============================================
class ParticleEmitterInstance
{
public:
    // このエミッターが参照するプリセット（設定データ）
    ParticlePreset* preset_ = nullptr;

    // 将来用：追加モジュール（ColorOverLife など）
    std::vector<std::unique_ptr<ParticleModule>> modules_;

    Transform transform_;                 // エミッターの位置・回転・スケール
    std::vector<Particle> particles_;     // このエミッターが持つ粒子

    // Spawn 用
    uint32_t spawnCount_ = 1;           // 1回のEmitで出す数
    float    spawnInterval_ = 0.1f;       // 連続発生間隔
    bool     spawnRepeat_ = false;      // ループさせるか
    float    spawnTimer_ = 0.0f;       // 間隔用タイマー

    // Update / Render 用の基本設定（Presetからコピーするイメージ）
    Vector3 baseVelocity_{};
    Vector3 baseRotationSpeed_{};
    Vector3 baseScaleSpeed_{};
    float   baseLifeTime_ = 1.0f;
    bool    useGravity_ = false;

    Vector4 baseColor_{ 1,1,1,1 };
    bool    billboard_ = true;
    bool    flipY_ = false;

public:
    ParticleEmitterInstance() = default;

    // プリセットの設定（後で manager から呼び出す想定）
    void Initialize(ParticlePreset* presetRef);

    // 一度の Emit でパーティクルを生成する
    // （内部的には preset_ の Spawn モジュールを見る想定）
    void Emit();

    // 1回だけまとめて出す
    void EmitOnce();

    // 毎フレーム更新
    void Update(float dt);

    // 描画処理
    // ※ 実際の描画は今はまだ ParticleManager にあるので、
    //    当面は「頂点バッファへ流し込むための情報を用意する」程度に留める
    void Draw();
};

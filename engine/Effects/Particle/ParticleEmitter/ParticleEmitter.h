#pragma once
#include "ParticleManager.h"
#include "Transform.h"

class ParticleEmitter
{
public:
    // コンストラクタ
    ParticleEmitter(ParticleManager* particleManager, const std::string& name, const Transform& transform, uint32_t count, float frequency, bool repeat = false);

    // 更新
    void Update();

    // Emit処理
    void Emit();

    // 繰り返し設定
    void SetRepeat(bool repeat);

private:
    ParticleManager* particleManager_; // ParticleManagerのインスタンスを保持
    std::string name_;                 // パーティクルグループ名
    Transform transform_;              // エミッターの位置・回転・スケール
    uint32_t count_;                   // 発生させるパーティクルの数
    float frequency_;                  // 発生頻度
    float elapsedTime_;                // 経過時間
    bool repeat_;                      // 繰り返し発生させるかどうかのフラグ
};

#pragma once
#include "AnimationClip.h"
#include "Quaternion.h"
#include "AnimationCurve.h"
#include <unordered_map>

class Animator {
public:
    // アニメーションを追加
    void AddAnimation(const std::string& name, const AnimationClip& clip);

    // 再生を開始
    void PlayAnimation(const std::string& name, bool loop = true);

    // アニメーション更新
    void Update(float deltaTime);

    // パレット取得（シェーダーへ渡す用）
    const std::vector<Matrix4x4>& GetCurrentPalette() const;

private:
    std::unordered_map<std::string, AnimationClip> animations_;
    const AnimationClip* currentClip_ = nullptr;

    float currentTime_ = 0.0f;
    bool isLooping_ = true;

    std::vector<Matrix4x4> currentPalette_;
};

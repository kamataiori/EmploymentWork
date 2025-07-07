#include "Animator.h"
#include <cassert>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

void Animator::AddAnimation(const std::string& name, const AnimationClip& clip) {
    animations_[name] = clip;
}

void Animator::PlayAnimation(const std::string& name, bool loop) {
    auto it = animations_.find(name);
    assert(it != animations_.end() && "Animation not found!");
    currentClip_ = &it->second;
    currentTime_ = 0.0f;
    isLooping_ = loop;
}

void Animator::Update(float deltaTime) {
    if (!currentClip_) return;

    currentTime_ += deltaTime;
    if (isLooping_) {
        currentTime_ = fmod(currentTime_, currentClip_->duration);
    }
    else {
        currentTime_ = std::min(currentTime_, currentClip_->duration);
    }

    const auto& clip = *currentClip_;

    // ボーン数の決定（nodeNameが"0","1",...前提）
    size_t maxIndex = 0;
    for (const auto& [nodeName, _] : clip.translate) {
        maxIndex = std::max(maxIndex, size_t(atoi(nodeName.c_str()) + 1));
    }

    currentPalette_.resize(maxIndex);

    for (const auto& [nodeName, curveT] : clip.translate) {
        Vector3 t = curveT.Evaluate(currentTime_);
        Vector3 s = clip.scale.at(nodeName).Evaluate(currentTime_);
        Quaternion r = clip.rotate.at(nodeName).Evaluate(currentTime_);

        Matrix4x4 matS = Matrix4x4::CreateScale(s);
        Matrix4x4 matR = Matrix4x4::CreateFromQuaternion(r);
        Matrix4x4 matT = Matrix4x4::CreateTranslation(t);

        Matrix4x4 localMatrix = matS * matR * matT;

        size_t index = size_t(atoi(nodeName.c_str())); // nodeNameが"0","1",...の前提
        currentPalette_[index] = localMatrix;
    }
}

const std::vector<Matrix4x4>& Animator::GetCurrentPalette() const {
    return currentPalette_;
}

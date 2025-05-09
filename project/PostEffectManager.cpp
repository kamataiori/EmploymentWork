#include "PostEffectManager.h"

PostEffectManager* PostEffectManager::GetInstance() {
    static PostEffectManager instance;
    return &instance;
}

void PostEffectManager::Initialize(PostEffectType type) {
    postEffect_ = std::make_unique<PostEffect>();
    postEffect_->Initialize(type);
}

void PostEffectManager::Draw() {
    if (postEffect_) {
        postEffect_->Draw();
    }
}

void PostEffectManager::Finalize()
{
    if (postEffect_) {
        postEffect_.reset();  // PostEffect（OffscreenRendering含む）を解放
    }
}

void PostEffectManager::SetType(PostEffectType type) {
    if (postEffect_ && postEffect_->GetType() == type) {
        return; // 変更なしならスキップ
    }
    postEffect_->SetType(type);
}

PostEffectType PostEffectManager::GetType() const {
    if (postEffect_) {
        return postEffect_->GetType();
    }
    return PostEffectType::Normal; // デフォルト値
}

uint32_t PostEffectManager::GetSrvIndex() const {
    return postEffect_ ? postEffect_->GetSrvIndex() : 0;
}

OffscreenRendering* PostEffectManager::GetOffscreen() {
    return postEffect_ ? postEffect_->GetOffscreen() : nullptr;
}

void PostEffectManager::SetVignetteScale(float scale)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetVignetteScale(scale);
    }
}

void PostEffectManager::SetVignettePower(float power)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetVignettePower(power);
    }
}

void PostEffectManager::SetVignetteColor(const Vector3& color)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetVignetteColor(color);
    }
}

void PostEffectManager::SetGrayscaleStrength(float strength)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetGrayscaleStrength(strength);
    }
}

void PostEffectManager::SetGrayscaleWeights(const Vector3& weights)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetGrayscaleWeights(weights);
    }
}

void PostEffectManager::SetSepiaColor(const Vector3& sepiaColor)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetSepiaColor(sepiaColor);
    }
}

void PostEffectManager::SetSepiaStrength(float strength)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetSepiaStrength(strength);
    }
}

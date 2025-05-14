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

void PostEffectManager::VignetteInitialize(float scale, float power, const Vector3& color)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->VignetteInitialize(scale, power, color);
    }
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

void PostEffectManager::GrayscaleInitialize(float strength, const Vector3& weights)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->GrayscaleInitialize(strength, weights);
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

void PostEffectManager::SepiaInitialize(const Vector3& sepiaColor, float strength)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SepiaInitialize(sepiaColor, strength);
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

void PostEffectManager::RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->RadialBlurInitialize(center, blurWidth, numSamples);
    }
}

void PostEffectManager::SetRadialBlurCenter(const Vector2& center)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRadialBlurCenter(center);
    }
}

void PostEffectManager::SetRadialBlurWidth(float blurWidth)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRadialBlurWidth(blurWidth);
    }
}

void PostEffectManager::SetRadialBlurSamples(int numSamples)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRadialBlurSamples(numSamples);
    }
}

void PostEffectManager::RandomInitialize(float time, float scale, float intensity)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->RandomInitialize(time, scale, intensity);
    }
}

void PostEffectManager::RandomUpdate(float deltaTime)
{
    if (postEffect_ && postEffect_->GetType() == PostEffectType::Random) {
        randomTime_ += deltaTime;
        postEffect_->GetOffscreen()->SetRandomTime(randomTime_);
    }
}

void PostEffectManager::SetRandomTime(float time)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRandomTime(time);
    }
}

void PostEffectManager::SetRandomScale(float scale)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRandomScale(scale);
    }
}

void PostEffectManager::SetRandomIntensity(float intensity)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRandomIntensity(intensity);
    }
}

void PostEffectManager::SetRandomUseImage(bool useImage)
{
    if (postEffect_ && postEffect_->GetOffscreen()) {
        postEffect_->GetOffscreen()->SetRandomUseImage(useImage);
    }
}

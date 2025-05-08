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

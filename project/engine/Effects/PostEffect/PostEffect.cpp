#include "PostEffect.h"

void PostEffect::Initialize(PostEffectType type) {
    offscreen_->Initialize(type);
}

void PostEffect::Draw() {
    offscreen_->Draw();
}

void PostEffect::SetType(PostEffectType type) {
    offscreen_->SetPostEffectType(type);
}

PostEffectType PostEffect::GetType() const {
    return offscreen_->GetPostEffectType();
}

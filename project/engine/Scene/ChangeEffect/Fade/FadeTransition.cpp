#include "FadeTransition.h"

void FadeTransition::Initialize(
    const std::string& texturePath,
    const Vector2& screenSize,
    int layer
)
{
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(texturePath);
    sprite_->SetSize(screenSize);
    sprite_->SetPosition({ 0.0f, 0.0f });

    alpha_ = 0.0f;
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();

    SetLayer(layer);
    SetActive(false);
}

void FadeTransition::Draw()
{
    if (!IsActive()) {
        return;
    }
    if (sprite_) {
        sprite_->Draw();
    }
}

void FadeTransition::OnFadeOut(float progress01)
{
    // 0 -> 1 で暗く
    alpha_ = progress01;
    ApplyAlpha();
}

void FadeTransition::OnFadeIn(float progress01)
{
    // 0 -> 1 で明るく（alphaは 1 -> 0）
    alpha_ = 1.0f - progress01;
    ApplyAlpha();
}

void FadeTransition::ApplyAlpha()
{
    if (!sprite_) return;

    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();
}

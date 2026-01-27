#pragma once
#include "SceneTransitionBase.h"
#include "Sprite.h"
#include <memory>
#include <string>

// 画面全体を黒でフェードする遷移演出
class FadeTransition final : public SceneTransitionBase {
public:
    FadeTransition() = default;
    ~FadeTransition() override = default;

    // 初期化
    // texturePath: 黒画像
    // screenSize : 画面サイズ
    // layer      : UIの最前面にしたいので大きい値を推奨
    void Initialize(
        const std::string& texturePath = "Resources/Black.png",
        const Vector2& screenSize = { 1280.0f, 720.0f },
        int layer = 100000
    );

    // UIElement
    void Draw() override;

protected:
    void OnFadeOut(float progress01) override;
    void OnFadeIn(float progress01) override;

private:
    void ApplyAlpha();

private:
    std::unique_ptr<Sprite> sprite_;
    float alpha_ = 0.0f;
};

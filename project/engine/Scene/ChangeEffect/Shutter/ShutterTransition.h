#pragma once
#include "engine/Scene/ChangeEffect/SceneTransitionBase.h"
#include "Sprite.h"
#include <memory>
#include <string>

// 上下からシャッターのように閉じる遷移
// Sprite 2枚（上板・下板）を動かして演出する
class ShutterTransition final : public SceneTransitionBase {
public:
    ShutterTransition() = default;
    ~ShutterTransition() override = default;

    // texturePath : 黒画像（1x1白でも可。SetColorで黒にしてもOK）
    // screenSize  : 画面サイズ（基本 1280x720）
    // layer       : 最前面
    void Initialize(
        const std::string& texturePath = "Resources/Black.png",
        const Vector2& screenSize = { 1280.0f, 720.0f },
        int layer = 100000
    );

    // UIElement
    void Draw() override;

protected:
    // 0→1 で閉じる
    void OnFadeOut(float progress01) override;

    // 0→1 で開く
    void OnFadeIn(float progress01) override;

private:
    // progress01(0..1) に応じて板位置をセットする
    // close=true なら「閉じる向き」, close=false なら「開く向き」
    void ApplyShutter(float progress01, bool close);

private:
    Vector2 screenSize_{ 1280.0f, 720.0f };

    std::unique_ptr<Sprite> top_;    // 上の板
    std::unique_ptr<Sprite> bottom_; // 下の板
};

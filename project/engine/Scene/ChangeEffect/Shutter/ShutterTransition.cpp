#include "ShutterTransition.h"
#include <algorithm>
#include "engine/TweenEasing.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

void ShutterTransition::Initialize(
    const std::string& texturePath,
    const Vector2& screenSize,
    int layer
)
{
    screenSize_ = screenSize;

    // 上板
    top_ = std::make_unique<Sprite>();
    top_->Initialize(texturePath);
    top_->SetAnchorPoint({ 0.0f, 0.0f });               // 左上基準
    top_->SetSize({ screenSize_.x, screenSize_.y * 0.5f }); // 画面の上半分
    top_->SetPosition({ 0.0f, -screenSize_.y * 0.5f }); // 最初は画面外上
    top_->Update();

    // 下板
    bottom_ = std::make_unique<Sprite>();
    bottom_->Initialize(texturePath);
    bottom_->SetAnchorPoint({ 0.0f, 0.0f });                 // 左上基準
    bottom_->SetSize({ screenSize_.x, screenSize_.y * 0.5f });   // 画面の下半分
    bottom_->SetPosition({ 0.0f, screenSize_.y });            // 最初は画面外下
    bottom_->Update();

    // 最前面
    SetLayer(layer);

    // Startされるまで非表示
    SetActive(false);

    // 初期状態（開いている＝板が外）
    ApplyShutter(0.0f, true);
}

void ShutterTransition::Draw()
{
    if (!IsActive()) return;

    // 上→下の順で描く（順序はどちらでもOK）
    if (top_) top_->Draw();
    if (bottom_) bottom_->Draw();
}

void ShutterTransition::OnFadeOut(float progress01)
{
    // 閉じる
    ApplyShutter(progress01, true);
}

void ShutterTransition::OnFadeIn(float progress01)
{
    // 開く（閉じるの逆）
    ApplyShutter(progress01, false);
}

void ShutterTransition::ApplyShutter(float progress01, bool close)
{
    float p = std::clamp(progress01, 0.0f, 1.0f);

    // 開くときは逆再生にしたいので反転
    float t = close ? p : (1.0f - p);

    // ここでイージングを適用（好きなものに変えてOK）
    // QuadよりCubicの方が「勢い」が出て気持ちいいことが多い
    const auto easing = Tween::Easing::EaseInOutCubic;
    t = easing(t);

    // 上板：画面外上 -> y=0（上半分を覆う位置）
    const float topStartY = -screenSize_.y * 0.5f;
    const float topEndY = 0.0f;

    // 下板：画面外下 -> y=画面中央（下半分を覆う位置）
    const float bottomStartY = screenSize_.y;
    const float bottomEndY = screenSize_.y * 0.5f;

    const float topY = Tween::Lerp(topStartY, topEndY, t);
    const float bottomY = Tween::Lerp(bottomStartY, bottomEndY, t);

    if (top_) {
        top_->SetPosition({ 0.0f, topY });
        top_->Update();
    }
    if (bottom_) {
        bottom_->SetPosition({ 0.0f, bottomY });
        bottom_->Update();
    }
}
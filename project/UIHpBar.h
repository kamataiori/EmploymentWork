#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include <memory>
#include <string>

class UIHpBar : public UIElement {
public:
    // ===== 生成用構造体 =====
    struct CreateDesc {
        std::string bgTexPath;     // 背景テクスチャパス
        std::string fillTexPath;   // ゲージテクスチャパス
        Vector2 pos{ 0.0f, 0.0f };   // 画面座標
        Vector2 size{ 200.0f, 20.0f };
        Vector2 anchor{ 0.5f, 0.5f };
        Vector4 bgColor{ 0.2f, 0.2f, 0.2f, 0.8f };   // 背景暗め
        Vector4 fillColor{ 1.0f, 1.0f, 1.0f, 1.0f }; // ゲージ明るめ
        int layer = 100;
    };

    static std::unique_ptr<UIHpBar> Create(const CreateDesc& desc);

    void ApplyData(const UIData& data) override;

    // ===== 値は外から渡す =====
    void SetHp(float hp, float maxHp);
    void SetRate(float rate01);

    // 位置変更（追従用など）
    void SetPosition(const Vector2& pos);

    void Update() override;
    void Draw() override;

private:
    UIHpBar() = default;

private:
    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<Sprite> fill_;

    Vector2 size_{ 200.0f, 20.0f };
    float rate01_ = 1.0f; // 0.0f〜1.0f
};

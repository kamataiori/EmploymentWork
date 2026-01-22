// UITexture.h
#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include <memory>
#include <string>

class UITexture : public UIElement {
public:
    struct Desc {
        std::string texPath;
        Vector2 pos{ 0,0 };
        Vector2 size{ 600,120 };
        Vector2 anchor{ 0.5f, 0.5f };
        Vector4 color{ 1,1,1,1 };
        int layer = 100;
    };

    static std::unique_ptr<UITexture> Create(const Desc& d);

    void SetTexture(const std::string& texPath);   // 表示切替用
    void SetPosition(const Vector2& pos);
    void SetColor(const Vector4& c);

    void Update() override;
    void Draw() override;

private:
    UITexture() = default;

    std::unique_ptr<Sprite> sprite_;
    Desc desc_;
};

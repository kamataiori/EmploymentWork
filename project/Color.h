#pragma once
#include <DirectXMath.h>
#include <unordered_map>
#include <string>

enum class Color {
    WHITE,
    RED,
    BLUE,
    GREEN,
    YELLOW,
    ORANGE,
    CYAN,
    PURPLE,
    LIME,
    BLACK,
    PINK
};

// 色の名前と対応する値を取得する関数
inline DirectX::XMFLOAT4 GetColorValue(Color color) {
    static const std::unordered_map<Color, DirectX::XMFLOAT4> colorMap = {
        {Color::WHITE,  {1.0f, 1.0f, 1.0f, 1.0f}},
        {Color::RED,    {1.0f, 0.0f, 0.0f, 1.0f}},
        {Color::BLUE,   {0.0f, 0.0f, 1.0f, 1.0f}},
        {Color::GREEN,  {0.0f, 1.0f, 0.0f, 1.0f}},
        {Color::YELLOW, {1.0f, 1.0f, 0.0f, 1.0f}},
        {Color::ORANGE, {1.0f, 0.5f, 0.0f, 1.0f}}, // オレンジ
        {Color::CYAN,   {0.0f, 1.0f, 1.0f, 1.0f}}, // 水色
        {Color::PURPLE, {0.5f, 0.0f, 0.5f, 1.0f}}, // 紫
        {Color::LIME,   {0.5f, 1.0f, 0.0f, 1.0f}}, // 黄緑
        {Color::BLACK,  {0.0f, 0.0f, 0.0f, 1.0f}}, // 黒
        {Color::PINK,   {1.0f, 0.5f, 0.5f, 1.0f}}  // ピンク
    };

    auto it = colorMap.find(color);
    if (it != colorMap.end()) {
        return it->second;
    }

    // デフォルトで白を返す
    return { 1.0f, 1.0f, 1.0f, 1.0f };
}

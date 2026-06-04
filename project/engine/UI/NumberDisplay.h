// NumberDisplay.h
#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

//======================================================
// NumberDisplay（数字PNGを並べて整数を表示する）
//------------------------------------------------------
// ・Resources/number/00.png 〜 09.png（0〜9）を桁ごとに並べて表示する。
// ・値は ApplyData(UIData{hp,maxHp}) から取得（Source で hp / maxHp を選ぶ）。
//   もしくは SetValue() で直接渡す（Source::Static）。
// ・align で基準位置を左端／右端に切り替えられる（右寄せにすると桁数が
//   変わっても右端が固定されるので、隣のUIがガタつかない）。
// ・ヘッダオンリー：新規 .cpp を増やさず UIManager/UIElement に乗せる。
//======================================================
class NumberDisplay : public UIElement {
public:
    enum class Source { Static, CurrentHp, MaxHp };
    enum class Align  { Left, Right };

    struct CreateDesc {
        std::string dir = "Resources/number/"; // 末尾に "0X.png" を付ける
        Vector2 pos{ 0.0f, 0.0f };  // 基準位置（縦中央）。align により左端/右端の意味が変わる
        Vector2 digitSize{ 24.0f, 32.0f };
        float   spacing = 2.0f;     // 桁間のすき間
        Align   align = Align::Left;
        Source  source = Source::Static;
        int     value = 0;          // Source::Static の初期値
        int     layer = 100;
    };

    static std::unique_ptr<NumberDisplay> Create(const CreateDesc& d) {
        std::unique_ptr<NumberDisplay> ui(new NumberDisplay());
        ui->desc_ = d;
        ui->SetLayer(d.layer);
        ui->Rebuild(d.value);
        return ui;
    }

    void SetValue(int v) {
        if (v < 0) v = 0;
        if (v == value_ && built_) return; // 変化なしなら作り直さない（テクスチャ再読込を避ける）
        Rebuild(v);
    }

    void ApplyData(const UIData& data) override {
        switch (desc_.source) {
        case Source::CurrentHp: SetValue(static_cast<int>(data.hp));    break;
        case Source::MaxHp:     SetValue(static_cast<int>(data.maxHp)); break;
        case Source::Static:    default: break;
        }
    }

    void Update() override {}

    void Draw() override {
        for (auto& sp : digits_) {
            sp->Draw();
        }
    }

private:
    NumberDisplay() = default;

    // 値を桁スプライト列に作り直す
    void Rebuild(int v) {
        value_ = v;
        built_ = true;
        digits_.clear();

        const std::string str = std::to_string(v); // 例 "275"
        const int n = static_cast<int>(str.size());

        const float totalW = n * desc_.digitSize.x + (n - 1) * desc_.spacing;
        // align によって先頭桁の左端 x を決める
        const float leftX = (desc_.align == Align::Right)
            ? (desc_.pos.x - totalW)   // 右端を pos.x に合わせる
            : desc_.pos.x;             // 左端を pos.x に合わせる

        digits_.reserve(n);
        for (int i = 0; i < n; ++i) {
            const char c = str[i];                       // '0'〜'9'
            const std::string path = desc_.dir + "0" + std::string(1, c) + ".png"; // 0→00.png …

            auto sp = std::make_unique<Sprite>();
            sp->Initialize(path);
            sp->SetSize(desc_.digitSize);
            sp->SetAnchorPoint({ 0.0f, 0.5f });          // 左端・縦中央
            const float px = leftX + i * (desc_.digitSize.x + desc_.spacing);
            sp->SetPosition({ px, desc_.pos.y });
            sp->Update();
            digits_.push_back(std::move(sp));
        }
    }

    CreateDesc desc_;
    std::vector<std::unique_ptr<Sprite>> digits_;
    int  value_ = 0;
    bool built_ = false;
};

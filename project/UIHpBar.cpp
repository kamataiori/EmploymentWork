#include "UIHpBar.h"
#include <algorithm>

//==============================
// 生成
//==============================
std::unique_ptr<UIHpBar> UIHpBar::Create(const CreateDesc& desc)
{
    std::unique_ptr<UIHpBar> ui(new UIHpBar());

    // --- 背景 ---
    ui->bg_ = std::make_unique<Sprite>();
    ui->bg_->Initialize(desc.bgTexPath);
    ui->bg_->SetPosition(desc.pos);
    ui->bg_->SetSize(desc.size);
    ui->bg_->SetAnchorPoint(desc.anchor);

    // --- ゲージ ---
    ui->fill_ = std::make_unique<Sprite>();
    ui->fill_->Initialize(desc.fillTexPath);
    ui->fill_->SetPosition(desc.pos);
    ui->fill_->SetSize(desc.size);
    ui->fill_->SetAnchorPoint(desc.anchor);

    ui->size_ = desc.size;
    ui->SetLayer(desc.layer);

    ui->SetPosition(desc.pos);
    ui->bg_->SetColor(desc.bgColor);
    ui->fill_->SetColor(desc.fillColor);

    ui->bg_->Update();
    ui->fill_->Update();

    return ui;
}

void UIHpBar::ApplyData(const UIData& data)
{
    OutputDebugStringA("UIHpBar::ApplyData called\n");

    SetHp(data.hp, data.maxHp);
}

//==============================
// HP値設定
//==============================
void UIHpBar::SetHp(float hp, float maxHp)
{
    float rate = (maxHp > 0.0f) ? (hp / maxHp) : 0.0f;
    SetRate(rate);
}

void UIHpBar::SetRate(float rate01)
{
    rate01_ = std::clamp(rate01, 0.0f, 1.0f);
}

//==============================
// 位置変更
//==============================
void UIHpBar::SetPosition(const Vector2& pos)
{
    bg_->SetPosition(pos);
    fill_->SetPosition(pos);

    bg_->Update();
    fill_->Update();
}

//==============================
// 更新・描画
//==============================
void UIHpBar::Update()
{
    Vector2 fillSize = size_;
    fillSize.x *= rate01_;
    fill_->SetSize(fillSize);

    bg_->Update();
    fill_->Update();
}

void UIHpBar::Draw()
{
    bg_->Draw();
    fill_->Draw();
}

#include "UITexture.h"

std::unique_ptr<UITexture> UITexture::Create(const Desc& d)
{
    std::unique_ptr<UITexture> ui(new UITexture());
    ui->desc_ = d;

    ui->sprite_ = std::make_unique<Sprite>();
    ui->sprite_->Initialize(d.texPath);
    ui->sprite_->SetAnchorPoint(d.anchor);
    ui->sprite_->SetSize(d.size);
    ui->sprite_->SetPosition(d.pos);
    ui->sprite_->SetColor(d.color);
    ui->sprite_->Update();

    ui->SetLayer(d.layer);
    return ui;
}

void UITexture::SetTexture(const std::string& texPath)
{
    desc_.texPath = texPath;
    sprite_->Initialize(texPath);      // あなたのSpriteが「再Initialize可」前提
    sprite_->SetAnchorPoint(desc_.anchor);
    sprite_->SetSize(desc_.size);
    sprite_->SetPosition(desc_.pos);
    sprite_->SetColor(desc_.color);
    sprite_->Update();
}

void UITexture::SetPosition(const Vector2& pos)
{
    desc_.pos = pos;
    sprite_->SetPosition(pos);
}

void UITexture::SetColor(const Vector4& c)
{
    desc_.color = c;
    sprite_->SetColor(c);
}

void UITexture::Update()
{
    sprite_->Update();
}

void UITexture::Draw()
{
    sprite_->Draw();
}

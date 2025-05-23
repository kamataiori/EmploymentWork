#include "Card.h"

Card::Card(Suit suit, Rank rank, const std::string& texturePath)
    : suit_(suit), rank_(rank), texturePath_(texturePath) {
    sprite_ = std::make_unique<Sprite>();
}

void Card::Initialize() {
    sprite_->Initialize(texturePath_);
    sprite_->SetSize({ 64.0f, 89.0f }); // 一般的なカード比率
}

void Card::Update() {
    sprite_->Update();
}

void Card::Draw() {
    sprite_->Draw();
}

void Card::SetPosition(const Vector2& pos) {
    sprite_->SetPosition(pos);
}

Suit Card::GetSuit() const {
    return suit_;
}

Rank Card::GetRank() const {
    return rank_;
}

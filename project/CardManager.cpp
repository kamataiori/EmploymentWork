#include "CardManager.h"
#include <random>
#include <algorithm>

void CardManager::Initialize() {
    deck_.clear();
    currentIndex_ = 0;

    for (int s = 0; s < 4; ++s) {
        for (int r = 1; r <= 13; ++r) {
            Suit suit = static_cast<Suit>(s);
            Rank rank = static_cast<Rank>(r);

            std::string path = "Resources/Ayahuda_Card/";

            switch (suit) {
            case Suit::Spade:   path += "Spade_"; break;
            case Suit::Heart:   path += "Heart_"; break;
            case Suit::Diamond: path += "Dia_";   break;
            case Suit::Club:    path += "Clover_"; break;
            }

            path += std::to_string(r);
            path += ".png";

            auto card = std::make_unique<Card>(suit, rank, path);
            card->Initialize();
            deck_.push_back(std::move(card));
        }
    }
}


void CardManager::Shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(deck_.begin(), deck_.end(), g);
    currentIndex_ = 0;
}

Card* CardManager::DrawCard() {
    if (currentIndex_ >= deck_.size()) return nullptr;
    return deck_[currentIndex_++].get();
}

void CardManager::Update() {
    for (auto& card : deck_) card->Update();
}

void CardManager::Draw() {
    for (auto& card : deck_) card->Draw();
}

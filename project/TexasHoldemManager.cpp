#include "TexasHoldemManager.h"

void TexasHoldemManager::Initialize() {
    cardManager_.Initialize();
    cardManager_.Shuffle();
    DealHands();
}

void TexasHoldemManager::NextPhase() {
    if (currentPhase_ == Phase::PreFlop) {
        currentPhase_ = Phase::Flop;
        DealCommunity();
    }
    else if (currentPhase_ == Phase::Flop) {
        currentPhase_ = Phase::Turn;
        DealCommunity();
    }
    else if (currentPhase_ == Phase::Turn) {
        currentPhase_ = Phase::River;
        DealCommunity();
    }
    else if (currentPhase_ == Phase::River) {
        currentPhase_ = Phase::Showdown;
    }
}

void TexasHoldemManager::DealHands() {
    playerHands_.clear();
    for (int i = 0; i < 2; ++i) {
        Card* card = cardManager_.DrawCard();
        card->SetPosition({ float(100 + i * 70), 500.0f }); // 画面下部
        playerHands_.push_back(card);
    }
}

void TexasHoldemManager::DealCommunity() {
    int numToAdd = 0;
    if (currentPhase_ == Phase::Flop) numToAdd = 3;
    if (currentPhase_ == Phase::Turn || currentPhase_ == Phase::River) numToAdd = 1;

    for (int i = 0; i < numToAdd; ++i) {
        Card* card = cardManager_.DrawCard();
        card->SetPosition({ float(300 + communityCards_.size() * 70), 300.0f }); // 中央に並べる
        communityCards_.push_back(card);
    }
}

void TexasHoldemManager::Update() {
    cardManager_.Update();
}

void TexasHoldemManager::Draw() {
    for (auto* card : communityCards_) card->Draw();
    for (auto* card : playerHands_) card->Draw();
}

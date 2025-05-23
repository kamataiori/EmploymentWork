#pragma once
#include "CardManager.h"
#include "HandRank.h"

enum class Phase {
    PreFlop,
    Flop,
    Turn,
    River,
    Showdown
};

class TexasHoldemManager {
public:
    void Initialize();
    void NextPhase();
    void DealHands();
    void DealCommunity();
    void EvaluatePlayerHand();

    void Update();
    void Draw();

    HandRank GetCurrentHandRank() const { return currentHandRank_; }

private:
    CardManager cardManager_;
    Phase currentPhase_ = Phase::PreFlop;

    std::vector<Card*> playerHands_;
    std::vector<Card*> communityCards_;

    HandRank currentHandRank_ = HandRank::HighCard;
};

#pragma once
#include "CardManager.h"

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

    void Update();
    void Draw();

private:
    CardManager cardManager_;
    Phase currentPhase_ = Phase::PreFlop;

    std::vector<Card*> playerHands_;
    std::vector<Card*> communityCards_;
};

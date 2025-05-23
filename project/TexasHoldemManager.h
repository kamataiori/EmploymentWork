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
    void EvaluateHands();
    void Reset();

    void Update();
    void Draw();

    HandRank GetCurrentHandRank() const { return playerRank_; }
    HandRank GetCpuHandRank() const { return cpuRank_; }
    Phase GetCurrentPhase() const { return currentPhase_; }

private:
    CardManager cardManager_;
    Phase currentPhase_ = Phase::PreFlop;

    std::vector<Card*> playerHands_;
    std::vector<Card*> cpuHands_;
    std::vector<Card*> communityCards_;

    HandRank playerRank_ = HandRank::HighCard;
    HandRank cpuRank_ = HandRank::HighCard;
};

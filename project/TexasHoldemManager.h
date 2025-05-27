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

struct PlayerState {
    int chips = 1000;
    int currentBet = 0;
    bool hasFolded = false;
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
    std::string GetWinnerName() const { return winnerName_; }

public:

    void StartBetting();       // ベットフェーズ開始
    void HandlePlayerAction(const std::string& action); // Check, Bet, Raise, Fold
    void HandleCpuAction();    // CPUの自動行動
    bool IsBettingPhase() const { return bettingPhase_; }
    void StartCheatPrompt();


    int GetPot() const { return pot_; }
    int GetPlayerChips() const { return player_.chips; }
    int GetCpuChips() const { return cpu_.chips; }


private:
    CardManager cardManager_;
    Phase currentPhase_ = Phase::PreFlop;

    std::vector<Card*> playerHands_;
    std::vector<Card*> cpuHands_;
    std::vector<Card*> communityCards_;

    HandRank playerRank_ = HandRank::HighCard;
    HandRank cpuRank_ = HandRank::HighCard;

    std::string winnerName_ = "";

    // プレイヤーとCPUの状態
    PlayerState player_;
    PlayerState cpu_;

    // ポット（合計賭け金）
    int pot_ = 0;

    // アクション制御
    bool isPlayerTurn_ = true;
    bool bettingPhase_ = true; // ベット中かどうか

    bool isGameOver_ = false;

    std::vector<Card*> pendingCards_;

    int playerBetAmount_ = 100; // プレイヤーがベット・レイズする金額（スライダーで変更）

    const float cardWidth = 128.0f;
    const float cardHeight = 178.0f;
    const float communityCardY = 300.0f;
    const float playerCardY = 500.0f;
    const float cpuCardY = 100.0f;

    bool cheatPromptActive_ = false;
    bool waitingForCheatResolution_ = false;

    /*bool cheatChoiceMade_ = false;
    bool cheatChosenCheat_ = false;*/

};

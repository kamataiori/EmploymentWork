#include "TexasHoldemManager.h"

void TexasHoldemManager::Initialize() {
    cardManager_.Initialize();
    cardManager_.Shuffle();
    DealHands();
}

void TexasHoldemManager::DealHands() {
    playerHands_.clear();
    cpuHands_.clear();

    for (int i = 0; i < 2; ++i) {
        Card* playerCard = cardManager_.DrawCard();
        playerCard->SetPosition({ 100.0f + i * 70, 500.0f });
        playerHands_.push_back(playerCard);

        Card* cpuCard = cardManager_.DrawCard();
        cpuHands_.push_back(cpuCard); // 位置設定なし（最初は非表示）
    }
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
        DealCommunity(); // ←これが必要なら維持

        // CPUカードの表示位置
        for (int i = 0; i < 2; ++i) {
            cpuHands_[i]->SetPosition({ 1100.0f + i * 70, 30.0f });
        }
    }

    // 各フェーズ終了後にプレイヤーのハンド評価を更新
    std::vector<Card*> playerTotal = playerHands_;
    playerTotal.insert(playerTotal.end(), communityCards_.begin(), communityCards_.end());
    playerRank_ = EvaluateHand(playerTotal);

    // ショーダウンでCPUも評価
    if (currentPhase_ == Phase::Showdown) {
        std::vector<Card*> cpuTotal = cpuHands_;
        cpuTotal.insert(cpuTotal.end(), communityCards_.begin(), communityCards_.end());
        cpuRank_ = EvaluateHand(cpuTotal);

        if (static_cast<int>(playerRank_) > static_cast<int>(cpuRank_)) {
            winnerName_ = "Player Wins!";
        }
        else if (static_cast<int>(playerRank_) < static_cast<int>(cpuRank_)) {
            winnerName_ = "CPU Wins!";
        }
        else {
            winnerName_ = "Draw!";
        }
    }
}

void TexasHoldemManager::DealCommunity() {
    int numToAdd = 0;
    if (currentPhase_ == Phase::Flop) numToAdd = 3;
    if (currentPhase_ == Phase::Turn || currentPhase_ == Phase::River) numToAdd = 1;

    for (int i = 0; i < numToAdd; ++i) {
        Card* card = cardManager_.DrawCard();
        card->SetPosition({ float(300 + communityCards_.size() * 70), 300.0f });
        communityCards_.push_back(card);
    }
}

void TexasHoldemManager::EvaluateHands() {
    std::vector<Card*> playerTotal = playerHands_;
    playerTotal.insert(playerTotal.end(), communityCards_.begin(), communityCards_.end());
    playerRank_ = EvaluateHand(playerTotal);

    std::vector<Card*> cpuTotal = cpuHands_;
    cpuTotal.insert(cpuTotal.end(), communityCards_.begin(), communityCards_.end());
    cpuRank_ = EvaluateHand(cpuTotal);

    // 勝敗判定
    if (static_cast<int>(playerRank_) > static_cast<int>(cpuRank_)) {
        winnerName_ = "Player Wins!";
    }
    else if (static_cast<int>(playerRank_) < static_cast<int>(cpuRank_)) {
        winnerName_ = "CPU Wins!";
    }
    else {
        winnerName_ = "Draw!";
    }
}

void TexasHoldemManager::Reset()
{
    // 全部初期化
    playerHands_.clear();
    cpuHands_.clear();
    communityCards_.clear();
    playerRank_ = HandRank::HighCard;
    cpuRank_ = HandRank::HighCard;
    currentPhase_ = Phase::PreFlop;

    cardManager_.Initialize();
    cardManager_.Shuffle();
    DealHands();
}

void TexasHoldemManager::Update() {
    cardManager_.Update();
}

void TexasHoldemManager::Draw() {
    for (auto* card : communityCards_) card->Draw();
    for (auto* card : playerHands_) card->Draw();

    if (currentPhase_ == Phase::Showdown) {
        for (auto* card : cpuHands_) card->Draw();
    }
}

#include "TexasHoldemManager.h"

void TexasHoldemManager::Initialize() {
    cardManager_.Initialize();
    cardManager_.Shuffle();
    DealHands();

    // 初期化
    player_ = PlayerState();
    cpu_ = PlayerState();
    pot_ = 0;
    isPlayerTurn_ = true;
    bettingPhase_ = true;
    isGameOver_ = false;  // 初期化でゲームオーバー状態を解除

}

void TexasHoldemManager::DealHands() {
    playerHands_.clear();
    cpuHands_.clear();

    for (int i = 0; i < 2; ++i) {
        Card* playerCard = cardManager_.DrawCard();
        playerCard->SetPosition({ 100.0f + i * (cardWidth + 10.0f), playerCardY });
        playerHands_.push_back(playerCard);

        Card* cpuCard = cardManager_.DrawCard();
        cpuHands_.push_back(cpuCard);
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

        for (int i = 0; i < 2; ++i) {
            cpuHands_[i]->SetPosition({ 100.0f + i * (cardWidth + 10.0f), cpuCardY });
        }
    }

    std::vector<Card*> playerTotal = playerHands_;
    playerTotal.insert(playerTotal.end(), communityCards_.begin(), communityCards_.end());
    playerRank_ = EvaluateHand(playerTotal);

    if (currentPhase_ == Phase::Showdown) {
        std::vector<Card*> cpuTotal = cpuHands_;
        cpuTotal.insert(cpuTotal.end(), communityCards_.begin(), communityCards_.end());
        cpuRank_ = EvaluateHand(cpuTotal);

        if (player_.hasFolded) {
            winnerName_ = "CPU Wins by Fold!";
            cpu_.chips += pot_;
        }
        else if (cpu_.hasFolded) {
            winnerName_ = "Player Wins by Fold!";
            player_.chips += pot_;
        }
        else if (static_cast<int>(playerRank_) > static_cast<int>(cpuRank_)) {
            winnerName_ = "Player Wins!";
            player_.chips += pot_;
        }
        else if (static_cast<int>(playerRank_) < static_cast<int>(cpuRank_)) {
            winnerName_ = "CPU Wins!";
            cpu_.chips += pot_;
        }
        else {
            winnerName_ = "Draw!";
            player_.chips += pot_ / 2;
            cpu_.chips += pot_ / 2;
        }

        if (player_.chips <= 0 || cpu_.chips <= 0) {
            isGameOver_ = true;
        }
    }
    else {
        StartBetting();
    }
}

void TexasHoldemManager::DealCommunity() {
    int numToAdd = 0;
    if (currentPhase_ == Phase::Flop) numToAdd = 3;
    if (currentPhase_ == Phase::Turn || currentPhase_ == Phase::River) numToAdd = 1;

    for (int i = 0; i < numToAdd; ++i) {
        Card* card = cardManager_.DrawCard();
        card->SetPosition({ float(300 + communityCards_.size() * (cardWidth + 10.0f)), communityCardY });
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
    winnerName_.clear();
    pot_ = 0;
    player_ = PlayerState();
    cpu_ = PlayerState();
    isPlayerTurn_ = true;
    bettingPhase_ = true;
    isGameOver_ = false;

    cardManager_.Initialize();
    cardManager_.Shuffle();
    DealHands();
}

void TexasHoldemManager::Update() {
    cardManager_.Update();

    static bool cheatChoiceMade_ = false;
    static bool cheatChosenCheat_ = false;

    // イカサマUI表示
    if (cheatPromptActive_ && !isGameOver_ && !cheatChoiceMade_) {
        ImGui::Begin("CheatChoice", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(520, 400));
        ImGui::SetWindowSize(ImVec2(250, 100));
        ImGui::Text("イカサマをしますか？");

        if (ImGui::Button("イカサマする")) {
            cheatChoiceMade_ = true;
            cheatChosenCheat_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("イカサマしない")) {
            cheatChoiceMade_ = true;
            cheatChosenCheat_ = false;
        }
        ImGui::End();
        return;
    }

    // イカサマの選択結果処理（1フレーム遅延）
    if (cheatChoiceMade_) {
        cheatPromptActive_ = false;
        cheatChoiceMade_ = false;

        if (cheatChosenCheat_) {
            waitingForCheatResolution_ = true;
        }
        else {
            NextPhase();
        }
    }

    // イカサマ実行処理
    if (waitingForCheatResolution_) {
        waitingForCheatResolution_ = false;
        if (rand() % 2 == 0) { // 成功
            cpu_.currentBet = static_cast<int>(cpu_.currentBet * 1.5f);
        }
        NextPhase();
    }

    ImGui::Begin("Chips", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos(ImVec2(500, 100));
    ImGui::SetWindowSize(ImVec2(300, 100));
    ImGui::Text("Player: %d", player_.chips);
    ImGui::Text("CPU: %d", cpu_.chips);
    ImGui::Text("POT: %d", pot_);
    ImGui::End();

    if (isGameOver_) {
        ImGui::Begin("GameOver", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(540, 160));
        ImGui::SetWindowSize(ImVec2(200, 60));
        if (player_.chips <= 0) {
            ImGui::Text("GAME OVER: CPU Wins");
        }
        else {
            ImGui::Text("GAME OVER: Player Wins");
        }
        ImGui::End();
    }

    if (bettingPhase_ && isPlayerTurn_ && !isGameOver_) {
        ImGui::Begin("PlayerAction", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(500, 600));
        ImGui::SetWindowSize(ImVec2(400, 150));

        ImGui::SliderInt("Amount", &playerBetAmount_, 0, player_.chips);

        if (ImGui::Button("Check")) HandlePlayerAction("Check");
        ImGui::SameLine();
        if (ImGui::Button("Bet")) HandlePlayerAction("Bet");
        ImGui::SameLine();
        if (ImGui::Button("Raise")) HandlePlayerAction("Raise");
        ImGui::SameLine();
        if (ImGui::Button("Fold")) HandlePlayerAction("Fold");

        ImGui::End();
    }
}



void TexasHoldemManager::Draw() {
    for (auto* card : communityCards_) card->Draw();
    for (auto* card : playerHands_) card->Draw();
    if (currentPhase_ == Phase::Showdown) {
        for (auto* card : cpuHands_) card->Draw();
    }
}

void TexasHoldemManager::StartBetting() {
    bettingPhase_ = true;
    isPlayerTurn_ = true;
    player_.currentBet = 0;
    cpu_.currentBet = 0;
    StartCheatPrompt();
}

void TexasHoldemManager::HandlePlayerAction(const std::string& action) {
    if (!isPlayerTurn_ || !bettingPhase_ || player_.hasFolded) return;

    if (action == "Check") {
        isPlayerTurn_ = false;
        HandleCpuAction();
    }
    else if (action == "Bet") {
        int amount = playerBetAmount_;
        if (player_.chips >= amount) {
            player_.chips -= amount;
            player_.currentBet = amount;
            pot_ += amount;
            isPlayerTurn_ = false;
            HandleCpuAction();
        }
    }
    else if (action == "Raise") {
        int amount = playerBetAmount_;
        if (player_.chips >= amount) {
            player_.chips -= amount;
            player_.currentBet = amount;
            pot_ += amount;
            isPlayerTurn_ = false;
            HandleCpuAction();
        }
    }
    else if (action == "Fold") {
        player_.hasFolded = true;
        bettingPhase_ = false;
        NextPhase();
    }
}

void TexasHoldemManager::HandleCpuAction() {
    if (cpu_.chips >= player_.currentBet) {
        cpu_.chips -= player_.currentBet;
        cpu_.currentBet = player_.currentBet;
        pot_ += cpu_.currentBet;
    }
    else {
        cpu_.hasFolded = true;
    }
    bettingPhase_ = false;
    NextPhase();
}

void TexasHoldemManager::StartCheatPrompt()
{
    cheatPromptActive_ = true;
}

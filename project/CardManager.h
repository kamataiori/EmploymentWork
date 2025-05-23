#pragma once
#include "Card.h"
#include <vector>

class CardManager {
public:
    void Initialize();
    void Shuffle();
    Card* DrawCard();

    void Update();
    void Draw();

private:
    std::vector<std::unique_ptr<Card>> deck_;
    size_t currentIndex_ = 0;
};

#pragma once
#include "Card.h"
#include <vector>

enum class HandRank {
    HighCard,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    RoyalFlush
};

const char* HandRankToString(HandRank rank);

HandRank EvaluateHand(const std::vector<Card*>& cards);

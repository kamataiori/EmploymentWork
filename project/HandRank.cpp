#include "HandRank.h"
#include <map>
#include <algorithm>

const char* HandRankToString(HandRank rank) {
    switch (rank) {
    case HandRank::HighCard: return "High Card";
    case HandRank::OnePair: return "One Pair";
    case HandRank::TwoPair: return "Two Pair";
    case HandRank::ThreeOfAKind: return "Three of a Kind";
    case HandRank::Straight: return "Straight";
    case HandRank::Flush: return "Flush";
    case HandRank::FullHouse: return "Full House";
    case HandRank::FourOfAKind: return "Four of a Kind";
    case HandRank::StraightFlush: return "Straight Flush";
    case HandRank::RoyalFlush: return "Royal Flush";
    default: return "";
    }
}

// 今はOnePair判定だけ簡単に実装
HandRank EvaluateHand(const std::vector<Card*>& cards) {
    std::map<int, int> rankCount;
    std::map<Suit, std::vector<int>> suits;
    std::vector<int> ranks;

    for (auto* card : cards) {
        int r = static_cast<int>(card->GetRank());
        Suit s = card->GetSuit();
        rankCount[r]++;
        suits[s].push_back(r);
        ranks.push_back(r);
    }

    std::sort(ranks.begin(), ranks.end());
    ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());

    // Ace can also be 1 for straight
    if (std::find(ranks.begin(), ranks.end(), 14) != ranks.end()) {
        ranks.insert(ranks.begin(), 1);
    }

    auto isStraight = [&](const std::vector<int>& r) {
        for (size_t i = 0; i + 4 < r.size(); ++i) {
            if (r[i] + 1 == r[i + 1] && r[i + 1] + 1 == r[i + 2] &&
                r[i + 2] + 1 == r[i + 3] && r[i + 3] + 1 == r[i + 4]) {
                return r[i + 4]; // highest card
            }
        }
        return 0;
        };

    // Flush
    for (auto& [suit, list] : suits) {
        if (list.size() >= 5) {
            std::sort(list.begin(), list.end());
            if (std::find(list.begin(), list.end(), 14) != list.end()) {
                list.insert(list.begin(), 1);
            }
            int highStraightFlush = isStraight(list);
            if (highStraightFlush) {
                if (highStraightFlush == 14) return HandRank::RoyalFlush;
                return HandRank::StraightFlush;
            }
            return HandRank::Flush;
        }
    }

    // Straight
    if (isStraight(ranks)) return HandRank::Straight;

    // Count same ranks
    bool hasThree = false, hasPair = false;
    for (auto& [rank, count] : rankCount) {
        if (count == 4) return HandRank::FourOfAKind;
        if (count == 3) hasThree = true;
        if (count == 2) {
            if (hasPair) return HandRank::TwoPair;
            hasPair = true;
        }
    }

    if (hasThree && hasPair) return HandRank::FullHouse;
    if (hasThree) return HandRank::ThreeOfAKind;
    if (hasPair) return HandRank::OnePair;

    return HandRank::HighCard;
}

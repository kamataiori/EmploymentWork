#pragma once
#include "Sprite.h"

enum class Suit { Spade, Heart, Diamond, Club };
enum class Rank {
    Two = 2, Three, Four, Five, Six,
    Seven, Eight, Nine, Ten,
    Jack, Queen, King, Ace
};

class Card {
public:
    Card(Suit suit, Rank rank, const std::string& texturePath);
    void Initialize();
    void Update();
    void Draw();

    void SetPosition(const Vector2& pos);

    Suit GetSuit() const;
    Rank GetRank() const;

private:
    Suit suit_;
    Rank rank_;
    std::unique_ptr<Sprite> sprite_;
    std::string texturePath_;
};

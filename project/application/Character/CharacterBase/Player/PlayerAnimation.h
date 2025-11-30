#pragma once
#include <unordered_map>
#include <string>
#include "PlayerAnimKey.h"
#include "PlayerType.h"

struct PlayerAnimKeyHash {
    size_t operator()(PlayerAnimKey k) const noexcept { return (size_t)k; }
};

class PlayerAnimation {
public:
    // 現在のプレイヤー種別に合わせてマッピングを組む
    void Use(PlayerType type);

    // 必要に応じて、外から個別上書きも可能
    void SetAlias(PlayerAnimKey key, const std::string& clip);

    // 共有キーから実クリップ名へ
    std::string Resolve(PlayerAnimKey key) const;

private:
    std::unordered_map<PlayerAnimKey, std::string, PlayerAnimKeyHash> map_;

    static std::string Or(const std::string& a, const std::string& b) {
        return a.empty() ? b : a;
    }

    void UseWarrior(); // Warrior.gltf 用の既定テーブル
    void UseRogue();   // Rogue.gltf   用の既定テーブル
};

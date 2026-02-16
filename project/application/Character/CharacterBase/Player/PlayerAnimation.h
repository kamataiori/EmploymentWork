#pragma once
#include <unordered_map>
#include <string>
#include "PlayerAnimKey.h"

struct PlayerAnimKeyHash {
    size_t operator()(PlayerAnimKey k) const noexcept { return static_cast<size_t>(k); }
};

class PlayerAnimation {
public:
    PlayerAnimation(); // 自動でWarriorテーブルを作る

    // 必要に応じて、外から個別上書きも可能
    void SetAlias(PlayerAnimKey key, const std::string& clip);

    // 共有キーから実クリップ名へ
    std::string Resolve(PlayerAnimKey key) const;

private:
    std::unordered_map<PlayerAnimKey, std::string, PlayerAnimKeyHash> map_;

    void BuildWarriorTable(); // 既定テーブル
};

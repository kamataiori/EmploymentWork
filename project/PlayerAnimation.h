#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <functional>
#include "PlayerAnimKey.h"
#include "AnimationSet.h"        // Warrior 用（Idle, Run_Weapon など）
#include "RogueAnimationSet.h"   // Rogue 用（Idle, Run など）

struct AnimKeyHash {
    size_t operator()(PlayerAnimKey k) const noexcept { return static_cast<size_t>(k); }
};

class PlayerAnimation {
public:
    // 文字列名を直接上書きしたいとき用（攻撃クラスから差し替え可能）
    void SetAlias(PlayerAnimKey key, const std::string& motionName);

    // 現在のプレイヤー（Warrior/Rogue）に合わせたマッピングを構築
    void UseWarrior(const AnimationSet& w);
    void UseRogue(const RogueAnimationSet& r);

    // 共通キーから実名を取得（見つからなければ空文字）
    std::string Resolve(PlayerAnimKey key) const;

private:
    std::unordered_map<PlayerAnimKey, std::string, AnimKeyHash> map_;

    // 安全フォールバック（空なら fallback を返す）
    static std::string Or(const std::string& primary, const std::string& fallback) {
        return primary.empty() ? fallback : primary;
    }
};

#include "PlayerAnimation.h"

void PlayerAnimation::Use(PlayerType type) {
    switch (type) {
    case PlayerType::Warrior: UseWarrior(); break;
    case PlayerType::Rogue:   UseRogue();   break;
    }
}

void PlayerAnimation::SetAlias(PlayerAnimKey key, const std::string& clip) {
    map_[key] = clip;
}

std::string PlayerAnimation::Resolve(PlayerAnimKey key) const {
    auto it = map_.find(key);
    if (it == map_.end()) return {};
    return it->second;
}

// ---- ここが “モデルごとの実クリップ名” の置き場所 ----
// ※ 必要ならあとから好きに編集可能（FBX/GLTFの実名に揃える）

void PlayerAnimation::UseWarrior() {
    map_.clear();

    // locomotion
    map_[PlayerAnimKey::Idle] = "Idle";
    map_[PlayerAnimKey::RunWeapon] = "Run_Weapon";
    map_[PlayerAnimKey::Run] = Or("Run", map_[PlayerAnimKey::RunWeapon]); // 無ければRun_Weaponで代用
    map_[PlayerAnimKey::Walk] = "Walk";
    map_[PlayerAnimKey::Roll] = "Roll";

    // combat
    map_[PlayerAnimKey::SwordAttack] = "Sword_Attack";
    map_[PlayerAnimKey::SwordAttackFast] = "Sword_AttackFast";
    map_[PlayerAnimKey::AttackA] = map_[PlayerAnimKey::SwordAttack];
    map_[PlayerAnimKey::AttackB] = map_[PlayerAnimKey::SwordAttackFast];
    map_[PlayerAnimKey::Punch] = "Punch";

    // others
    map_[PlayerAnimKey::IdleAttacking] = "Idle_Attacking";
    map_[PlayerAnimKey::PickUp] = "PickUp";
    map_[PlayerAnimKey::Hit1] = "RecieveHit";
    map_[PlayerAnimKey::Hit2] = "RecieveHit_2";
    map_[PlayerAnimKey::Death] = "Death";
}

void PlayerAnimation::UseRogue() {
    map_.clear();

    // locomotion
    map_[PlayerAnimKey::Idle] = "Idle";
    map_[PlayerAnimKey::Run] = "Run";
    map_[PlayerAnimKey::RunWeapon] = map_[PlayerAnimKey::Run]; // RogueはRun_Weaponが無い前提
    map_[PlayerAnimKey::Walk] = "Walk";
    map_[PlayerAnimKey::Roll] = "Roll";

    // combat
    map_[PlayerAnimKey::AttackA] = "Dagger_Attack";
    map_[PlayerAnimKey::AttackB] = "Dagger_Attack2";
    map_[PlayerAnimKey::SwordAttack] = map_[PlayerAnimKey::AttackA]; // 共通キーから見て代用
    map_[PlayerAnimKey::SwordAttackFast] = map_[PlayerAnimKey::AttackB];
    map_[PlayerAnimKey::Punch] = "Punch";

    // others
    map_[PlayerAnimKey::IdleAttacking] = "Attacking_Idle";
    map_[PlayerAnimKey::PickUp] = "PickUp";
    map_[PlayerAnimKey::Hit1] = "RecieveHit";
    map_[PlayerAnimKey::Hit2] = "RecieveHit_2";
    map_[PlayerAnimKey::Death] = "Death";
}

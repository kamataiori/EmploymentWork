#include "PlayerAnimation.h"

PlayerAnimation::PlayerAnimation() {
    BuildWarriorTable();
}

void PlayerAnimation::SetAlias(PlayerAnimKey key, const std::string& clip) {
    map_[key] = clip;
}

std::string PlayerAnimation::Resolve(PlayerAnimKey key) const {
    auto it = map_.find(key);
    if (it == map_.end()) return {};
    return it->second;
}

void PlayerAnimation::BuildWarriorTable() {
    map_.clear();

    // ----------------
    // locomotion
    // ----------------
    map_[PlayerAnimKey::Idle] = "Idle";
    map_[PlayerAnimKey::Walk] = "Walk";
    map_[PlayerAnimKey::Run] = "Run";
    map_[PlayerAnimKey::RunWeapon] = "RunWeapon";
    map_[PlayerAnimKey::Roll] = "Roll";

    // ----------------
    // combat
    // ----------------
    map_[PlayerAnimKey::Attack01] = "Attack01";
    map_[PlayerAnimKey::Attack02] = "Attack02";
    map_[PlayerAnimKey::Attack03] = "Attack03";
    map_[PlayerAnimKey::SwordAttack] = "SwordAttack";
    map_[PlayerAnimKey::SwordAttackFast] = "SwordAttackFast";
    map_[PlayerAnimKey::Punch] = "Punch";

    // ----------------
    // others
    // ----------------
    map_[PlayerAnimKey::IdleAttacking] = "IdleAttacking";
    map_[PlayerAnimKey::IdleWeapon] = "IdleWeapon";
    map_[PlayerAnimKey::PickUp] = "PickUp";
    map_[PlayerAnimKey::RecieveHit] = "RecieveHit";
    map_[PlayerAnimKey::RecieveHit2] = "RecieveHit2";
    map_[PlayerAnimKey::Death] = "Death";
}

#include "PlayerAnimation.h"

void PlayerAnimation::SetAlias(PlayerAnimKey key, const std::string& motionName) {
    map_[key] = motionName;
}

void PlayerAnimation::UseWarrior(const AnimationSet& w) {
    map_.clear();
    // Locomotion
    map_[PlayerAnimKey::Idle] = w.Idle;
    map_[PlayerAnimKey::IdleWeapon] = w.Idle_Weapon;
    map_[PlayerAnimKey::IdleAttacking] = w.Idle_Attacking;
    map_[PlayerAnimKey::Walk] = w.Walk;
    map_[PlayerAnimKey::Run] = w.Run;
    map_[PlayerAnimKey::RunWeapon] = Or(w.Run_Weapon, w.Run); // 無ければRunにフォールバック
    map_[PlayerAnimKey::Roll] = w.Roll;

    // Combat
    map_[PlayerAnimKey::Punch] = w.Punch;
    map_[PlayerAnimKey::SwordAttack] = w.Sword_Attack;
    map_[PlayerAnimKey::SwordAttackFast] = w.Sword_AttackFast;
    map_[PlayerAnimKey::AttackA] = w.Sword_Attack;
    map_[PlayerAnimKey::AttackB] = w.Sword_AttackFast;

    // Other
    map_[PlayerAnimKey::PickUp] = w.PickUp;
    map_[PlayerAnimKey::Hit1] = w.RecieveHit;
    map_[PlayerAnimKey::Hit2] = w.RecieveHit_2;
    map_[PlayerAnimKey::Death] = w.Death;
}

void PlayerAnimation::UseRogue(const RogueAnimationSet& r) {
    map_.clear();
    // Locomotion
    map_[PlayerAnimKey::Idle] = r.Idle;
    map_[PlayerAnimKey::IdleWeapon] = r.Idle;      // Rogueに武器別Idleが無ければIdleに寄せる
    map_[PlayerAnimKey::IdleAttacking] = r.Attacking_Idle;
    map_[PlayerAnimKey::Walk] = r.Walk;
    map_[PlayerAnimKey::Run] = r.Run;
    map_[PlayerAnimKey::RunWeapon] = Or(r.Run, r.Walk); // RogueはRun_Weaponが無い → Run or Walk
    map_[PlayerAnimKey::Roll] = r.Roll;

    // Combat
    map_[PlayerAnimKey::Punch] = r.Punch;
    map_[PlayerAnimKey::DaggerAttack] = r.Dagger_Attack;
    map_[PlayerAnimKey::DaggerAttack2] = r.Dagger_Attack2;
    map_[PlayerAnimKey::AttackA] = r.Dagger_Attack;
    map_[PlayerAnimKey::AttackB] = r.Dagger_Attack2;

    // Other
    map_[PlayerAnimKey::PickUp] = r.PickUp;
    map_[PlayerAnimKey::Hit1] = r.RecieveHit;
    map_[PlayerAnimKey::Hit2] = r.RecieveHit_2;
    map_[PlayerAnimKey::Death] = r.Death;
}

std::string PlayerAnimation::Resolve(PlayerAnimKey key) const {
    auto it = map_.find(key);
    if (it == map_.end()) { return {}; }
    return it->second;
}

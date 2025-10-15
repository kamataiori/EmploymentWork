#pragma once
#include <cstdint>

enum class PlayerAnimKey : uint32_t {
    // locomotion
    Idle,
    Run,
    RunWeapon,
    Roll,
    Walk,

    // combat
    SwordAttack,
    SwordAttackFast,
    AttackA,
    AttackB,
    Punch,

    // others
    IdleAttacking,
    PickUp,
    Hit1,
    Hit2,
    Death,
};

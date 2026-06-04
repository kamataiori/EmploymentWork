#pragma once
#include <cstdint>

enum class PlayerAnimKey : uint32_t {
    // locomotion
    Idle,
    Walk,
    Run,
    RunWeapon,
    Roll,

    // combat
    Attack01,
    Attack02,
    Attack03,
    SwordAttack,
    SwordAttackFast,
    Punch,

    // others
    IdleAttacking,
    IdleWeapon,
    PickUp,
    RecieveHit,
    RecieveHit2,
    Death,
};

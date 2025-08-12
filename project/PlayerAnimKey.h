#pragma once
#include <cstdint>

enum class PlayerAnimKey : uint32_t {
    // Locomotion
    Idle,
    IdleWeapon,
    IdleAttacking,
    Walk,
    Run,
    RunWeapon,
    Roll,

    // Combat (増えてもOK)
    AttackA,
    AttackB,
    SwordAttack,
    SwordAttackFast,
    DaggerAttack,
    DaggerAttack2,

    // Other
    Punch,
    PickUp,
    Hit1,
    Hit2,
    Death,
};

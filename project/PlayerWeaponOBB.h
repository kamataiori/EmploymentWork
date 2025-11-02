#pragma once
#include "PlayerIWeapon.h"
#include "OBBCollider.h"
#include "CollisionTypeIdDef.h"
#include <unordered_set>

class PlayerWeaponOBB : public PlayerIWeapon, public OBBCollider {
public:

	PlayerWeaponOBB() : OBBCollider(obb) {}

	void Initialize() override;

	void Update() override;

	void Draw() override;
   
	void NormalAttack() override;

	void Skill() override;

	void Ultimate() override;

private:

	bool IsnormalAttack_ = false;
	bool IsSkill_ = false;
};

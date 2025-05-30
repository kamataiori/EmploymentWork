#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "OBBCollider.h"

class Enemy : public CharacterBase, public SphereCollider
{
public:

	Enemy(BaseScene* baseScene_) : CharacterBase(baseScene_), SphereCollider(sphere) {}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void OnCollision() override;

	bool IsDead() const { return isDead_; }
	void SetDead(bool flag) { isDead_ = flag; }

private:

	bool isDead_ = false;
};


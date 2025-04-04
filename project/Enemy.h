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

private:
};


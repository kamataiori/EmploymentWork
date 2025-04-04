#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"

class Player : public CharacterBase, public SphereCollider
{
public:

	Player(BaseScene* baseScene_) : CharacterBase(baseScene_),SphereCollider(sphere){}

	void Initialize() override;

	void Update() override;

	void Draw() override;

private:

};


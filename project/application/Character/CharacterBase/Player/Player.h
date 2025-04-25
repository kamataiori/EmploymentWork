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

	void Debug() override;

private:

	inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
		return a + (b - a) * t;
	}

private:

	Vector3 velocity_{};


};


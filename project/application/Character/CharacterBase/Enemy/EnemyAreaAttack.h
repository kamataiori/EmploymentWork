#pragma once
#include "CharacterBase.h"
#include "SphereCollider.h"

class EnemyAreaAttack : public CharacterBase, public SphereCollider {
public:
	EnemyAreaAttack(BaseScene* scene);
	void Initialize() override;
	void Update() override;
	void Draw() override;
	void SkinningDraw() override;
	void ParticleDraw() override;
	void OnCollision() override;

	void SetLifetime(float time) {
		maxLifetime_ = time;
		//lifetime_ = 0.0f;
	}

	void SetTranslate(const Vector3& pos) { transform.translate = pos; }

	bool IsDead() const { return isDead_; }   // Getter
	void Kill() { isDead_ = true; }

private:
	float lifetime_ = 0.0f;
	float maxLifetime_ = 6.0f;
	bool isDead_ = false;
};

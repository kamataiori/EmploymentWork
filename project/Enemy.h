#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "OBBCollider.h"
#include "EnemyState.h"
#include <memory>
#include <EnemyAreaAttack.h>
#include <EnemyAttackBullet.h>

class Player; // 前方宣言

class Enemy : public CharacterBase, public SphereCollider
{
public:

	Enemy(BaseScene* baseScene_) : CharacterBase(baseScene_), SphereCollider(sphere) {}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void OnCollision() override;

	void ChangeState(std::unique_ptr<EnemyState> State);
	void ChangeToRandomState();
	void Move(const Vector3& velocity);

	void AddAreaAttack(std::unique_ptr<EnemyAreaAttack> attack);

	void AddBullet(std::unique_ptr<EnemyAttackBullet> bullet);

	const std::list<std::unique_ptr<EnemyAreaAttack>>& GetAreaAttacks() const {
		return areaAttacks_;
	}

	const std::list<std::unique_ptr<EnemyAttackBullet>>& GetAttackBulets() const {
		return bullets_;
	}

	void SetPlayer(Player* player) { player_ = player; }
	Vector3 GetPlayerPos() const;

	Camera* GetCamera() const { return camera_; }

private:

	std::unique_ptr<EnemyState> currentState_;
	std::string previousStateName_ = "";

	std::list<std::unique_ptr<EnemyAttackBullet>> bullets_;
	std::list<std::unique_ptr<EnemyAreaAttack>> areaAttacks_;

	Player* player_ = nullptr;
};


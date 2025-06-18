#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "OBBCollider.h"
#include "EnemyState.h"
#include <memory>

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
	void Move(const Vector3& velocity); // 追加

	void SetPlayer(Player* player) { player_ = player; }
	Vector3 GetPlayerPos() const;

private:

	std::unique_ptr<EnemyState> currentState_;

	Player* player_ = nullptr;
};


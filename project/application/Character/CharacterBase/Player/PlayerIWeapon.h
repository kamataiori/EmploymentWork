#pragma once
#include "Transform.h"
#include "MultiCollider.h"

class PlayerBase;

class PlayerIWeapon
{
public:

	virtual ~PlayerIWeapon() = default;

	//// 所有者とシーンを受け取って初期化
	//virtual void Initialize(PlayerBase* owner, BaseScene* scene) = 0;

	virtual void Initialize() = 0;

	//// 毎フレ更新（追従・寿命管理など）
	virtual void Update() = 0;

	// デバッグ描画など（必要なら）
	virtual void Draw() = 0;

	// 通常攻撃トリガ
	virtual void NormalAttack() = 0;

	/// <summary>
	/// スキル1
	/// </summary>
	virtual void Skill() = 0;

	/// <summary>
	/// アルティメット
	/// </summary>
	virtual void Ultimate() = 0;

	void SetPlayerTransform(Transform* transform) { playerTransform_ = transform; }

	void SetOwner(PlayerBase* owner) { owner_ = owner; }

protected:

	Transform* playerTransform_ = {};

	PlayerBase* owner_ = nullptr;


};


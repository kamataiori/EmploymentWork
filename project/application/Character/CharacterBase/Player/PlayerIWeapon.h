#pragma once
#include "Transform.h"
#include "MultiCollider.h"

class Player;
class IEnemyTargetProvider;

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

	// 前景（2D UI）の描画。アルティメットのロックオンUIなどをスキルへ委譲する。
	// デフォルトは何もしない（UIを持たない武器実装を壊さないよう非純粋）。
	virtual void ForeGroundDraw() {}

	// 通常攻撃トリガ
	virtual void NormalAttack() = 0;

	/// <summary>
	/// スキル1（E キー／回転斬り）
	/// </summary>
	virtual void Skill() = 0;

	/// <summary>
	/// スキル2（V キー／突進乱舞）
	/// </summary>
	virtual void Skill2() = 0;

	/// <summary>
	/// アルティメット（Q キー／内容は今後実装）
	/// </summary>
	virtual void Ultimate() = 0;

	/// <summary>
	/// スキル2（突進乱舞）が攻撃対象を引くための供給元を注入する。
	/// </summary>
	virtual void SetEnemyTargetProvider(IEnemyTargetProvider* provider) = 0;

	void SetPlayerTransform(Transform* transform) { playerTransform_ = transform; }

	void SetOwner(Player* owner) { owner_ = owner; }

protected:

	Transform* playerTransform_ = {};

	Player* owner_ = nullptr;


};


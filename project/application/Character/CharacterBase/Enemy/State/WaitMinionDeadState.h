#pragma once
#include "EnemyActionState.h"

//======================================================
// WaitMinionDeadState
//------------------------------------------------------
// 雑魚敵が全滅するまで定位置で待機するステート
//
// ・雑魚敵が全員死んだら完了（false を返す）
// ・全滅するまで永遠に待ち続ける
// ・待機中はプレイヤーの方向を向く
//======================================================
class WaitMinionDeadState : public EnemyActionState
{
public:
	WaitMinionDeadState() = default;

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "WaitMinionDead"; }

private:
};
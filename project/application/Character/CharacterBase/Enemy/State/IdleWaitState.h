#pragma once
#include "EnemyActionState.h"

//======================================================
// IdleWaitState
//------------------------------------------------------
// 一定時間その場で待機するステート
// ChaseTarget の代わりに Selector の最後に置くことで
// 攻撃の合間にリズムと「隙」を生み出す
//
// ・プレイヤーの方向だけ向く（移動しない）
// ・waitTime_ 秒後に Success を返す
//======================================================
class IdleWaitState : public EnemyActionState
{
public:
	// waitTime: 待機時間（秒）
	explicit IdleWaitState(float waitTime = 0.8f);

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "IdleWait"; }

private:
	float waitTime_ = 0.8f;
	float timer_ = 0.0f;
};
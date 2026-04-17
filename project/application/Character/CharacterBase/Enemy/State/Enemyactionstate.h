#pragma once

class Enemy;

//======================================================
// EnemyActionState（基底クラス）
//------------------------------------------------------
// BT が「何をするか」を選び、State が「どう実行するか」を担う
//
// ライフサイクル:
//   Enter()  → ステート開始時に1回
//   Update() → 毎フレーム（Running中）
//   Exit()   → ステート終了時に1回
//
// Update() の戻り値:
//   true  → まだ実行中（Running）
//   false → 完了（BTのLeafに Success を返す）
//======================================================
class EnemyActionState
{
public:
	virtual ~EnemyActionState() = default;

	// ステート開始時に1回呼ばれる
	virtual void Enter(Enemy* enemy) = 0;

	// 毎フレーム呼ばれる。true=実行中、false=完了
	virtual bool Update(Enemy* enemy, float dt) = 0;

	// ステート終了時に1回呼ばれる
	virtual void Exit(Enemy* enemy) = 0;

	// デバッグ用：ステート名を返す
	virtual const char* GetName() const = 0;
};
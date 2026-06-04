#pragma once
#include <memory>
#include "EnemyActionState.h"

class Enemy;

//======================================================
// EnemyStateManager
//------------------------------------------------------
// Enemy が持つステート管理クラス
//
// 使い方:
//   BT の Leaf から:
//     enemy->GetStateManager()->ChangeState(std::make_unique<SomeState>());
//
//   Enemy::Update() から:
//     stateManager_->Update(this, dt);
//
//   BT の Leaf から実行完了を確認:
//     if (enemy->GetStateManager()->IsFinished()) → Success
//======================================================
class EnemyStateManager
{
public:
	EnemyStateManager() = default;
	~EnemyStateManager() = default;

	// ステートを切り替える（前のステートの Exit → 新しいステートの Enter）
	void ChangeState(Enemy* enemy, std::unique_ptr<EnemyActionState> newState);

	// 毎フレーム呼ぶ
	void Update(Enemy* enemy, float dt);

	// 現在のステートが完了したか（BTのLeafが参照する）
	bool IsFinished() const { return isFinished_; }

	// 現在のステートが存在するか
	bool HasState() const { return currentState_ != nullptr; }

	// デバッグ用：現在のステート名
	const char* GetCurrentStateName() const;

private:
	std::unique_ptr<EnemyActionState> currentState_;
	bool isFinished_ = true;  // ステートがない状態は「完了」扱い
};
#include "EnemyStateManager.h"
#include "EnemyActionState.h"

void EnemyStateManager::ChangeState(Enemy* enemy, std::unique_ptr<EnemyActionState> newState)
{
	// 前のステートを終了
	if (currentState_) {
		currentState_->Exit(enemy);
	}

	// 新しいステートを開始
	currentState_ = std::move(newState);
	isFinished_ = false;

	if (currentState_) {
		currentState_->Enter(enemy);
	}
	else {
		isFinished_ = true;
	}
}

void EnemyStateManager::Update(Enemy* enemy, float dt)
{
	if (!currentState_ || isFinished_) {
		return;
	}

	// Update が false を返したら完了
	bool stillRunning = currentState_->Update(enemy, dt);

	if (!stillRunning) {
		currentState_->Exit(enemy);
		isFinished_ = true;
	}
}

const char* EnemyStateManager::GetCurrentStateName() const
{
	if (currentState_) {
		return currentState_->GetName();
	}
	return "None";
}
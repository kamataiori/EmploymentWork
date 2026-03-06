#include "ExecuteStateLeaf.h"
#include "Enemy/Enemy.h"
#include "application/Character/CharacterBase/Enemy/State/EnemyStateManager.h"
#include "application/Character/CharacterBase/Enemy/State/EnemyActionState.h"

ExecuteStateLeaf::ExecuteStateLeaf(BlackBoard* bb, StateFactory factory)
	: LeafNodeBase(bb)
	, stateFactory_(std::move(factory))
{
}

void ExecuteStateLeaf::init()
{
	NodeBase::init();

	// ステートを生成して Enemy の StateManager にセット
	Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
	if (!enemy || !stateFactory_) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	auto newState = stateFactory_();
	if (!newState) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	enemy->GetStateManager()->ChangeState(enemy, std::move(newState));
	mNodeResult = NodeResult::Running;
}

void ExecuteStateLeaf::tick()
{
	Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
	if (!enemy) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	// StateManager が完了を報告したら Success
	if (enemy->GetStateManager()->IsFinished()) {
		mNodeResult = NodeResult::Success;
	}
	else {
		mNodeResult = NodeResult::Running;
	}
}
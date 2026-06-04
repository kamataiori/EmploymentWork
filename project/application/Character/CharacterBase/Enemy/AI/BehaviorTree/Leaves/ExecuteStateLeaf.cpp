#include "ExecuteStateLeaf.h"
#include <string>
#include "Enemy/Enemy.h"
#include "application/Character/CharacterBase/Enemy/State/EnemyStateManager.h"
#include "application/Character/CharacterBase/Enemy/State/EnemyActionState.h"

ExecuteStateLeaf::ExecuteStateLeaf(BlackBoard* bb, StateFactory factory,
	const std::string& actionName)
	: LeafNodeBase(bb)
	, stateFactory_(std::move(factory))
	, actionName_(actionName)
{
}

void ExecuteStateLeaf::init()
{
	NodeBase::init();

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

	if (enemy->GetStateManager()->IsFinished()) {
		mNodeResult = NodeResult::Success;
		// 攻撃名を BlackBoard に記録（NotLastActionLeaf が参照）
		if (!actionName_.empty()) {
			mpBlackBoard->set_value<std::string>("last_action", actionName_);
		}
	}
	else {
		mNodeResult = NodeResult::Running;
	}
}
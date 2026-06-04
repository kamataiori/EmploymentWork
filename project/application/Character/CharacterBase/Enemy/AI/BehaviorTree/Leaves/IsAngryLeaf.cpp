#include "IsAngryLeaf.h"
#include "Enemy/Enemy.h"

IsAngryLeaf::IsAngryLeaf(BlackBoard* bb)
	: LeafNodeBase(bb)
{
}

void IsAngryLeaf::init()
{
	NodeBase::init();
}

void IsAngryLeaf::tick()
{
	Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
	if (!enemy) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	mNodeResult = enemy->IsAngry()
		? NodeResult::Success
		: NodeResult::Fail;
}
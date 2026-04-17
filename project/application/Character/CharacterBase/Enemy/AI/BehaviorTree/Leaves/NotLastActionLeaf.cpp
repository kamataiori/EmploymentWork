#include "NotLastActionLeaf.h"

NotLastActionLeaf::NotLastActionLeaf(BlackBoard* bb, const std::string& actionName)
	: LeafNodeBase(bb), actionName_(actionName)
{
}

void NotLastActionLeaf::init()
{
	NodeBase::init();
}

void NotLastActionLeaf::tick()
{
	std::string lastAction;
	if (mpBlackBoard->try_get_value<std::string>("last_action", lastAction)) {
		// 前回と同じ攻撃なら Fail（連続使用を禁止）
		if (lastAction == actionName_) {
			mNodeResult = NodeResult::Fail;
			return;
		}
	}
	// 前回と違う、または記録がない → Success（使用可能）
	mNodeResult = NodeResult::Success;
}
#include "IsHPLeaf.h"
#include "Enemy/Enemy.h"

IsHPLeaf::IsHPLeaf(BlackBoard* bb, int targetPhase)
	: LeafNodeBase(bb), targetPhase_(targetPhase)
{
}

void IsHPLeaf::init()
{
	NodeBase::init();
}

void IsHPLeaf::tick()
{
	Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
	if (!enemy) {
		mNodeResult = NodeResult::Fail;
		return;
	}

	// EnemyPhase を int で比較
	// Phase1=0, Phase2=1, Phase3=2
	int currentPhase = static_cast<int>(enemy->GetPhase());

	// targetPhase_ 以上のフェーズ（HPが低い）なら Success
	// 例: targetPhase_=2 → Phase2(1) or Phase3(2) なら Success
	mNodeResult = (currentPhase >= targetPhase_)
		? NodeResult::Success
		: NodeResult::Fail;
}
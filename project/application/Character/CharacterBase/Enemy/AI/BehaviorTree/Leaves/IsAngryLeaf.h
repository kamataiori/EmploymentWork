#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

//======================================================
// IsAngryLeaf
//------------------------------------------------------
// Enemy の HP が 50% 以下（怒り状態）なら Success
// 通常状態なら Fail
//
// BTでの使い方:
//   Sequence
//   ├─ IsAngryLeaf  ← 怒り状態でなければ Fail
//   └─ [怒り時専用行動]
//======================================================
class IsAngryLeaf : public LeafNodeBase
{
public:
	explicit IsAngryLeaf(BlackBoard* bb);

protected:
	void init() override;
	void tick() override;
};
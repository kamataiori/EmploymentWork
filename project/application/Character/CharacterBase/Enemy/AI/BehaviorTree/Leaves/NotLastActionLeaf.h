#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include <string>

//======================================================
// NotLastActionLeaf
//------------------------------------------------------
// BlackBoard の "last_action" キーを参照し、
// 前回と同じ攻撃なら Fail を返す Condition Leaf
//
// 使い方:
//   Sequence
//   ├─ NotLastActionLeaf(bb, "ChargeDash")  ← 前回ダッシュなら Fail
//   └─ ChargeDash
//
// BlackBoard のキー：
//   "last_action" : std::string  (ExecuteStateLeaf 側で書き込む)
//======================================================
class NotLastActionLeaf : public LeafNodeBase
{
public:
	// actionName: このLeafが守る攻撃名（例: "ChargeDash"）
	NotLastActionLeaf(BlackBoard* bb, const std::string& actionName);

protected:
	void init() override;
	void tick() override;

private:
	std::string actionName_;
};
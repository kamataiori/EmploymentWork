#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include <memory>
#include <functional>
#include <string>

class EnemyActionState;

//======================================================
// ExecuteStateLeaf
//------------------------------------------------------
// BT の Leaf として、任意の EnemyActionState を実行する
//
// 使い方:
//   auto leaf = std::make_unique<ExecuteStateLeaf>(
//       bb,
//       [param]() { return std::make_unique<ChargeDashState>(param); }
//   );
//
// ・init() でステートを生成して Enemy の StateManager にセット
// ・tick() で StateManager の完了を監視
// ・完了したら Success を返す
//======================================================
class ExecuteStateLeaf : public LeafNodeBase
{
public:
	// stateFactory: 呼ばれるたびに新しいステートを作って返す関数
	using StateFactory = std::function<std::unique_ptr<EnemyActionState>()>;

	// actionName: BlackBoard の "last_action" に書き込む名前
	// 省略すると記録しない（FindTarget 等の非攻撃 Leaf 用）
	ExecuteStateLeaf(BlackBoard* bb, StateFactory factory,
		const std::string& actionName = "");

protected:
	void init() override;
	void tick() override;

private:
	StateFactory stateFactory_;
	std::string  actionName_;   // BlackBoard に記録する攻撃名
};
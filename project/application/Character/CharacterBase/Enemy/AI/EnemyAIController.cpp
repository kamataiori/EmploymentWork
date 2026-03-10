#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// Composite
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"

// Leaf（従来）
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/StayHomeLeaf.h"

// Leaf + State
#include "BehaviorTree/Leaves/ExecuteStateLeaf.h"
#include <Enemy/State/ChargeDashState.h>
#include <Enemy/State/SummonMinionState.h>

void EnemyAIController::Initialize(Enemy* owner)
{
	owner_ = owner;

	blackboard_ = std::make_unique<BlackBoard>();

	blackboard_->set_value<Enemy*>("enemy", owner_);

	BuildTree();
}

void EnemyAIController::Update(float dt)
{
	if (!owner_ || !root_ || !blackboard_) { return; }

	blackboard_->set_value<float>("dt", dt);

	root_->execute();
}

void EnemyAIController::BuildTree()
{
	FixHysteresis();

	blackboard_->set_value("charge_dash_param", chargeDashParam_);

	// ★ ルートは Sequence: FindTarget → 攻撃パターン選択(Selector)
	auto rootSeq = std::make_unique<SequenceNode>(blackboard_.get());

	// ターゲット探索
	auto find = std::make_unique<FindTargetLeaf>(
		blackboard_.get(),
		[this]() -> const Transform*
		{
			if (targetGetter_) {
				const Transform* t = targetGetter_();
				if (t) return t;
			}
			return owner_ ? owner_->GetTargetTransform() : nullptr;
		}
	);
	rootSeq->add_node(std::move(find));

	// ★ 攻撃パターンを Selector で選択
	//    今は Sequence（順番に全部実行）にしておく
	//    将来的に条件分岐（距離判定など）を入れて Selector に変えられる
	auto attackSeq = std::make_unique<SequenceNode>(blackboard_.get());

	// パターン1: チャージダッシュ → 分裂弾
	ChargeDashParam p = chargeDashParam_;
	attackSeq->add_node(std::make_unique<ExecuteStateLeaf>(
		blackboard_.get(),
		[p]() { return std::make_unique<ChargeDashState>(p); }
	));

	// ★ パターン2: 雑魚敵召喚
	attackSeq->add_node(std::make_unique<ExecuteStateLeaf>(
		blackboard_.get(),
		[]() { return std::make_unique<SummonMinionState>(); }
	));

	rootSeq->add_node(std::move(attackSeq));

	root_ = std::move(rootSeq);
}

void EnemyAIController::FixHysteresis()
{
	const float kGap = 0.5f;

	if (attackDist_ < stopDist_) {
		attackDist_ = stopDist_;
	}
	if (chaseStartDist_ <= attackDist_ + kGap) {
		chaseStartDist_ = attackDist_ + kGap;
	}
}
#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// Composite
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"

// Leaf（従来）
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/StayHomeLeaf.h"

//　Leaf + State
#include <Enemy/State/ChargeDashState.h>
#include "BehaviorTree/Leaves/ExecuteStateLeaf.h"

void EnemyAIController::Initialize(Enemy* owner)
{
	owner_ = owner;

	blackboard_ = std::make_unique<BlackBoard>();

	// Leafが参照できるように Enemy を登録
	blackboard_->set_value<Enemy*>("enemy", owner_);

	BuildTree();
}

void EnemyAIController::Update(float dt)
{
	if (!owner_ || !root_ || !blackboard_) { return; }

	blackboard_->set_value<float>("dt", dt);

	// ツリー実行
	root_->execute();
}

void EnemyAIController::BuildTree()
{
	FixHysteresis();

	// 黒板へ突進パラメータを登録（従来互換）
	blackboard_->set_value("charge_dash_param", chargeDashParam_);

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

	// ★ ChargeDashAttackLeaf → ExecuteStateLeaf + ChargeDashState に置き換え
	//    ファクトリがパラメータをキャプチャして、呼ばれるたびに新しいステートを作る
	ChargeDashParam p = chargeDashParam_;
	rootSeq->add_node(std::make_unique<ExecuteStateLeaf>(
		blackboard_.get(),
		[p]() { return std::make_unique<ChargeDashState>(p); }
	));

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
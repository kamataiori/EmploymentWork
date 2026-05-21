#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// JSON 読み込み用
#include <json.hpp>
#include <fstream>
#include <algorithm>
#include <random>

// Composite
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/RandomSelectorNode.h"

// Decorator
#include "application/AI/BehaviorTree/Nodes/Decorator/InverterDecorator.h"

// Leaf
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/StayHomeLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/ChaseTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsTargetFarLeaf.h"

// Leaf + State
#include "BehaviorTree/Leaves/ExecuteStateLeaf.h"
#include <Enemy/State/ChargeDashState.h>
#include <Enemy/State/SummonMinionState.h>
#include "application/Character/CharacterBase/Enemy/State/SpinAttackState.h"
#include "application/Character/CharacterBase/Enemy/State/JumpSlamState.h"
#include "application/Character/CharacterBase/Enemy/State/FeintAttackState.h"
#include "application/Character/CharacterBase/Enemy/State/ShootSplitBulletState.h"
#include "application/Character/CharacterBase/Enemy/State/ThrowBigBulletState.h"
#include "application/Character/CharacterBase/Enemy/State/IdleWaitState.h"
#include "application/Character/CharacterBase/Enemy/State/WaitMinionDeadState.h"
#include "application/Character/CharacterBase/Enemy/State/ThrowBigBulletState.h"
#include "application/Character/CharacterBase/Enemy/State/IdleWaitState.h"
#include "application/Character/CharacterBase/Enemy/State/WaitMinionDeadState.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsHPLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NotLastActionLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsAngryLeaf.h"

// StateManager（デバッグ用）
#include "application/Character/CharacterBase/Enemy/State/EnemyStateManager.h"

void EnemyAIController::Initialize(Enemy* owner)
{
	owner_ = owner;
	blackboard_ = std::make_unique<BlackBoard>();
	blackboard_->set_value<Enemy*>("enemy", owner_);

	BuildTree();

	std::ifstream ifs("Resources/BT/Enemy/enemy_bt.json");
	if (ifs)
	{
		try {
			nlohmann::json j;
			ifs >> j;
			BTEditor::BTGraph graph = j.get<BTEditor::BTGraph>();
			RebuildFromGraph(graph);
		}
		catch (...) {}
	}
}

void EnemyAIController::Update(float dt)
{
	if (!owner_ || !root_ || !blackboard_) { return; }
	blackboard_->set_value<float>("dt", dt);
	root_->execute();

	// NodeBase::execute() が次フレーム冒頭で自動 Reset するため手動 Reset 不要
}

void EnemyAIController::BuildTree()
{
	FixHysteresis();
	blackboard_->set_value("charge_dash_param", chargeDashParam_);

	auto rootSeq = std::make_unique<SequenceNode>(blackboard_.get());

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

	auto attackSeq = std::make_unique<SequenceNode>(blackboard_.get());
	ChargeDashParam p = chargeDashParam_;
	attackSeq->add_node(std::make_unique<ExecuteStateLeaf>(
		blackboard_.get(),
		[p]() { return std::make_unique<ChargeDashState>(p); }
	));
	attackSeq->add_node(std::make_unique<ExecuteStateLeaf>(
		blackboard_.get(),
		[]() { return std::make_unique<SummonMinionState>(); }
	));
	rootSeq->add_node(std::move(attackSeq));
	root_ = std::move(rootSeq);
}

void EnemyAIController::RebuildFromGraph(const BTEditor::BTGraph& graph)
{
	using namespace BTEditor;

	if (!owner_ || !blackboard_) return;

	const EditorNode* rootNode = nullptr;
	for (auto& n : graph.nodes) {
		if (n.kind == NodeKind::Root) { rootNode = &n; break; }
	}
	if (!rootNode) return;

	blackboard_->set_value<ChargeDashParam>("charge_dash_param", chargeDashParam_);

	std::function<std::unique_ptr<INode>(int)> Build =
		[&](int nodeId) -> std::unique_ptr<INode>
		{
			const EditorNode* en = graph.FindNode(nodeId);
			if (!en) return nullptr;

			std::vector<int> children = graph.ChildrenOf(nodeId);

			switch (en->kind)
			{
			case NodeKind::Sequence:
			{
				auto seq = std::make_unique<SequenceNode>(blackboard_.get());
				for (int cid : children) {
					auto child = Build(cid);
					if (child) seq->add_node(std::move(child));
				}
				return seq;
			}
			case NodeKind::Selector:
			{
				auto sel = std::make_unique<SelectorNode>(blackboard_.get());
				for (int cid : children) {
					auto child = Build(cid);
					if (child) sel->add_node(std::move(child));
				}
				return sel;
			}
			case NodeKind::RandomSelector:
			{
				auto rsel = std::make_unique<RandomSelectorNode>(blackboard_.get());
				for (int cid : children) {
					auto child = Build(cid);
					if (child) rsel->add_node(std::move(child));
				}
				return rsel;
			}
			case NodeKind::Decorator:
			{
				auto dec = std::make_unique<InverterDecorator>(blackboard_.get());
				if (!children.empty()) {
					auto child = Build(children[0]);
					if (child) dec->set_node(std::move(child));
				}
				return dec;
			}
			case NodeKind::Root:
			{
				if (!children.empty()) return Build(children[0]);
				return nullptr;
			}
			case NodeKind::Leaf:
			{
				const NodeParam& p = en->param;
				switch (p.stateType)
				{
				case LeafStateType::FindTarget:
					return std::make_unique<FindTargetLeaf>(
						blackboard_.get(),
						[this]() -> const Transform* {
							if (targetGetter_) return targetGetter_();
							return owner_ ? owner_->GetTargetTransform() : nullptr;
						});
				case LeafStateType::ChaseTarget:
					return std::make_unique<ChaseTargetLeaf>(
						blackboard_.get(),
						stopDist_, chaseSpeed_, turnLerp_);
				case LeafStateType::NearIdle:
					return std::make_unique<NearIdleLeaf>(blackboard_.get());
				case LeafStateType::StayHome:
					return std::make_unique<StayHomeLeaf>(blackboard_.get(), turnLerp_);
				case LeafStateType::IsTargetFar:
					return std::make_unique<IsTargetFarLeaf>(
						blackboard_.get(), p.thresholdDistance);
				case LeafStateType::ChargeDash:
				{
					ChargeDashParam cp;
					cp.chargeTime = p.chargeDash.chargeTime;
					cp.dashDistance = p.chargeDash.dashDistance;
					cp.dashSpeed = p.chargeDash.dashSpeed;
					cp.recoverTime = p.chargeDash.recoverTime;
					cp.cooldownTime = p.chargeDash.cooldownTime;
					cp.turnLerp = p.chargeDash.turnLerp;
					// 怒り状態なら速度・溜め・クールダウンを強化
					{
						Enemy* _owner = blackboard_.get() ? blackboard_->get_value<Enemy*>("enemy") : nullptr;
						if (_owner && _owner->IsAngry()) {
							cp.dashSpeed *= 1.6f;  // 速度1.6倍
							cp.chargeTime *= 0.5f;  // 溜め半分
							cp.cooldownTime *= 0.6f;  // クールダウン短め
						}
					}
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[cp]() { return std::make_unique<ChargeDashState>(cp); },
						"ChargeDash");
				}
				case LeafStateType::SummonMinion:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<SummonMinionState>(); }, "SummonMinion");
				case LeafStateType::ShootSplitBullet:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<ShootSplitBulletState>(); }, "ShootSplitBullet");
				case LeafStateType::ThrowBigBullet:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<ThrowBigBulletState>(); }, "ThrowBigBullet");
				case LeafStateType::IsPhase2:
					// Phase1=0, Phase2=1, Phase3=2
					// Phase2以上（HP50%以下）なら Success
					return std::make_unique<IsHPLeaf>(blackboard_.get(), 1);
				case LeafStateType::IsPhase3:
					return std::make_unique<IsHPLeaf>(blackboard_.get(), 2);
				case LeafStateType::NotLastAction:
					return std::make_unique<NotLastActionLeaf>(
						blackboard_.get(), p.lastActionName);
				case LeafStateType::IsAngry:
					return std::make_unique<IsAngryLeaf>(blackboard_.get());
				case LeafStateType::ShootSplitBulletAngry:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<ShootSplitBulletState>(true); },
						"ShootSplitBullet");
				case LeafStateType::IdleWait:
				{
					// thresholdDistance を待機秒数として流用（0なら7秒デフォルト）
					float waitSec = (p.thresholdDistance > 0.0f) ? p.thresholdDistance : 7.0f;
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[waitSec]() { return std::make_unique<IdleWaitState>(waitSec); },
						"IdleWait");
				}
				case LeafStateType::SummonMinionNormal:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<SummonMinionState>(); },
						"SummonMinion");
				case LeafStateType::WaitMinionDead:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<WaitMinionDeadState>(); },
						"WaitMinionDead");
				case LeafStateType::SpinAttack:
				{
					// 回転薙ぎ払い（近接撃退技）
					SpinAttackParam sp;
					// 怒り状態なら溜め短縮・回転高速化で圧を強める
					{
						Enemy* _owner = blackboard_.get() ? blackboard_->get_value<Enemy*>("enemy") : nullptr;
						if (_owner && _owner->IsAngry()) {
							sp.windUpTime *= 0.6f;
							sp.spinSpeed *= 1.3f;
							sp.recoverTime *= 0.7f;
						}
					}
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[sp]() { return std::make_unique<SpinAttackState>(sp); },
						"SpinAttack");
				}
				case LeafStateType::JumpSlam:
				{
					// ジャンプ急降下叩きつけ（間合いを詰める追撃技）
					JumpSlamParam jp;
					// 怒り状態なら溜め短縮・跳躍高速化で圧を強める
					{
						Enemy* _owner = blackboard_.get() ? blackboard_->get_value<Enemy*>("enemy") : nullptr;
						if (_owner && _owner->IsAngry()) {
							jp.windUpTime *= 0.6f;
							jp.leapTime *= 0.8f;
							jp.recoverTime *= 0.7f;
						}
					}
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[jp]() { return std::make_unique<JumpSlamState>(jp); },
						"JumpSlam");
				}
				case LeafStateType::FeintAttack:
				{
					// フェイント攻撃（読み合い技）
					FeintAttackParam fp;
					// 怒り状態なら誘い・硬直を短縮し、より読みにくくする
					{
						Enemy* _owner = blackboard_.get() ? blackboard_->get_value<Enemy*>("enemy") : nullptr;
						if (_owner && _owner->IsAngry()) {
							fp.windUpTime *= 0.7f;
							fp.feintHoldTime *= 0.7f;
							fp.recoverTime *= 0.7f;
						}
					}
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[fp]() { return std::make_unique<FeintAttackState>(fp); },
						"FeintAttack");
				}
				default:
					return nullptr;
				}
			}
			default:
				return nullptr;
			}
		};

	auto newRoot = Build(rootNode->id);
	if (newRoot) {
		root_ = std::move(newRoot);
	}
}

EnemyAIController::BTDebugInfo EnemyAIController::GetDebugInfo() const
{
	BTDebugInfo info;

	info.rootResult = root_ ? root_->get_node_result() : NodeResult::Idle;

	if (owner_) {
		auto* sm = owner_->GetStateManager();
		if (sm && sm->HasState()) {
			info.runningStateName = sm->GetCurrentStateName()
				? sm->GetCurrentStateName() : "---";
		}
		else {
			info.runningStateName = sm && sm->IsFinished() ? "(完了待ち)" : "(なし)";
		}
	}

	if (blackboard_) {
		const Transform* target = nullptr;
		if (blackboard_->try_get_value<const Transform*>("target", target) && target) {
			char buf[128];
			snprintf(buf, sizeof(buf),
				"target=(%.1f, %.1f, %.1f)",
				target->translate.x,
				target->translate.y,
				target->translate.z);
			info.blackboardInfo = buf;
		}
		else {
			info.blackboardInfo = "target=null";
		}
	}

	return info;
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
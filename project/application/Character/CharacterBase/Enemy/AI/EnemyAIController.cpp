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
#include <json.hpp>
#include <fstream>
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/ChaseTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsTargetFarLeaf.h"
#include "application/AI/BehaviorTree/Nodes/Decorator/InverterDecorator.h"

void EnemyAIController::Initialize(Enemy* owner)
{
	owner_ = owner;

	blackboard_ = std::make_unique<BlackBoard>();

	blackboard_->set_value<Enemy*>("enemy", owner_);

	BuildTree(); // デフォルトツリー

	// エディターで保存した JSON が存在すれば上書き読み込み
	std::ifstream ifs("Resources/BT/Enemy/enemy_bt.json");
	if (ifs)
	{
		try {
			nlohmann::json j;
			ifs >> j;
			BTEditor::BTGraph graph = j.get<BTEditor::BTGraph>();
			RebuildFromGraph(graph);
		}
		catch (...)
		{
			// 読み込み失敗時はデフォルトツリーをそのまま使う
		}
	}
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

	//    攻撃パターンを Selector で選択
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

//======================================================
// RebuildFromGraph
// BTEditor::BTGraph → ランタイム INode ツリーを再構築
// エディターで保存した JSON をゲームに反映する
//======================================================
void EnemyAIController::RebuildFromGraph(const BTEditor::BTGraph& graph)
{
	using namespace BTEditor;

	if (!owner_ || !blackboard_) return;

	// ルートノードを探す
	const EditorNode* rootNode = nullptr;
	for (auto& n : graph.nodes) {
		if (n.kind == NodeKind::Root) { rootNode = &n; break; }
	}
	if (!rootNode) return;

	blackboard_->set_value<ChargeDashParam>("charge_dash_param", chargeDashParam_);

	// 再帰でノードツリーを構築するラムダ
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
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[cp]() { return std::make_unique<ChargeDashState>(cp); });
				}
				case LeafStateType::SummonMinion:
					return std::make_unique<ExecuteStateLeaf>(
						blackboard_.get(),
						[]() { return std::make_unique<SummonMinionState>(); });
				default:
					return nullptr;
				}
			}
			default:
				return nullptr;
			}
		}; // end Build

	auto newRoot = Build(rootNode->id);
	if (newRoot) {
		root_ = std::move(newRoot);
	}
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
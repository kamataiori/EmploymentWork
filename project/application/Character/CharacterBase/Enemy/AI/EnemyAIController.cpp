#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// Composite
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"

// Leaf
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/IsTargetFarLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/ChaseTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include <PunchAttackLeaf.h>
#include <IsTargetNearLeaf.h>

void EnemyAIController::Initialize(Enemy* owner)
{
    owner_ = owner;

    // Blackboard作成
    blackboard_ = std::make_unique<BlackBoard>();

    // Leafが参照できるように Enemy を登録
    blackboard_->set_value<Enemy*>("enemy", owner_);

    BuildTree();
}

void EnemyAIController::Update(float dt)
{
    if (!owner_ || !root_ || !blackboard_) { return; }

    // 必要なら dt をBlackBoardに入れておく（将来Wait/Cooldown等で使える）
    blackboard_->set_value<float>("dt", dt);

    // ツリー実行
    root_->execute();
}

void EnemyAIController::BuildTree()
{
    // ヒステリシス修正（距離の整合性を保つ）
    FixHysteresis();

    // Root: Sequence
    auto rootSeq = std::make_unique<SequenceNode>(blackboard_.get());

    //==================================================
    // 1) FindTarget（ターゲットTransformを黒板へ）
    //==================================================
    auto find = std::make_unique<FindTargetLeaf>(
        blackboard_.get(),
        [this]() -> const Transform*
        {
            // Scene注入getterがあれば優先
            if (targetGetter_) {
                const Transform* t = targetGetter_();
                if (t) return t;
            }
            // Enemyが保持しているtargetをfallback
            return owner_ ? owner_->GetTargetTransform() : nullptr;
        }
    );
    rootSeq->add_node(std::move(find));

    //==================================================
    // 2) Selector: Near->Punch / Far->Chase / Idle
    //==================================================
    auto selector = std::make_unique<SelectorNode>(blackboard_.get());

    // -----------------------------------------------
    // Near branch: Sequence( IsNear -> Punch )
    // ※Selectorは「上から順に試す」ので、近接攻撃を先に置く
    // -----------------------------------------------
    {
        // 攻撃開始距離（例：attackDist_ を攻撃距離として使う）
        const float attackDist = attackDist_;

        auto punchSeq = std::make_unique<SequenceNode>(blackboard_.get());
        punchSeq->add_node(std::make_unique<IsTargetNearLeaf>(blackboard_.get(), attackDist));

        // パンチ：duration=0.6秒 / hit=0.15秒 / cooldown=0.5秒
        punchSeq->add_node(std::make_unique<PunchAttackLeaf>(blackboard_.get(), 0.6f, 0.15f, 0.5f));

        selector->add_node(std::move(punchSeq));
    }

    // -----------------------------------------------
    // Far branch: Sequence( IsFar -> Chase )
    // -----------------------------------------------
    {
        auto chaseSeq = std::make_unique<SequenceNode>(blackboard_.get());
        chaseSeq->add_node(std::make_unique<IsTargetFarLeaf>(blackboard_.get(), chaseStartDist_));
        chaseSeq->add_node(std::make_unique<ChaseTargetLeaf>(blackboard_.get(), stopDist_, chaseSpeed_, turnLerp_));

        selector->add_node(std::move(chaseSeq));
    }

    // -----------------------------------------------
    // Fallback: Idle（どちらでもない時）
    // -----------------------------------------------
    selector->add_node(std::make_unique<NearIdleLeaf>(blackboard_.get()));

    rootSeq->add_node(std::move(selector));

    root_ = std::move(rootSeq);
}


void EnemyAIController::FixHysteresis()
{
    // 追跡開始 > 攻撃開始 > 追跡停止（例）
    const float kGap = 0.5f;

    if (attackDist_ < stopDist_) {
        attackDist_ = stopDist_; // 追跡停止より攻撃開始が短いと変になる
    }
    if (chaseStartDist_ <= attackDist_ + kGap) {
        chaseStartDist_ = attackDist_ + kGap;
    }
}

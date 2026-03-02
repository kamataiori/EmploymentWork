#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// Composite
#include "application/AI/BehaviorTree/Nodes/Composite/SequenceNode.h"
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"

// Leaf
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/FindTargetLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/NearIdleLeaf.h"
#include "application/Character/CharacterBase/Enemy/AI/BehaviorTree/Leaves/StayHomeLeaf.h"
#include "BehaviorTree/Leaves/ChargeDashAttackLeaf.h"

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
    FixHysteresis();

    // ここで黒板へ突進パラメータを登録
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

    // Leaf はパラメータのキー名だけ知っていればOK
    rootSeq->add_node(std::make_unique<ChargeDashAttackLeaf>(
        blackboard_.get(),
        "charge_dash_param"
    ));

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

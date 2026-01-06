#include "EnemyAIController.h"
#include "Enemy/Enemy.h"

// Composite（あなたが作ったやつ）
#include "SequenceNode.h"
#include "SelectorNode.h"

// Leaf（今回追加したやつ）
#include "FindTargetLeaf.h"
#include "IsTargetFarLeaf.h"
#include "ChaseTargetLeaf.h"
#include "NearIdleLeaf.h"

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
    // ヒステリシス修正
    FixHysteresis();

    // Root: Sequence
    auto rootSeq = std::make_unique<SequenceNode>(blackboard_.get());

    // 1) FindTarget（getter優先、無ければEnemy側のtargetを使う作りにする）
    //    → getterが無い場合に備え、owner_->GetTargetTransform() を fallback させる
    auto find = std::make_unique<FindTargetLeaf>(
        blackboard_.get(),
        [this]() -> const Transform*
        {
            // Scene注入getter
            if (targetGetter_) {
                const Transform* t = targetGetter_();
                if (t) return t;
            }
            // Enemyが保持しているtarget
            return owner_ ? owner_->GetTargetTransform() : nullptr;
        }
    );

    rootSeq->add_node(std::move(find));

    // 2) Selector: Far->Chase / Near->Idle
    auto selector = std::make_unique<SelectorNode>(blackboard_.get());

    // Far branch: Sequence( IsFar -> Chase )
    auto chaseSeq = std::make_unique<SequenceNode>(blackboard_.get());
    chaseSeq->add_node(std::make_unique<IsTargetFarLeaf>(blackboard_.get(), chaseStartDist_));
    chaseSeq->add_node(std::make_unique<ChaseTargetLeaf>(blackboard_.get(), stopDist_, chaseSpeed_, turnLerp_));

    selector->add_node(std::move(chaseSeq));

    // Near branch: Idle
    selector->add_node(std::make_unique<NearIdleLeaf>(blackboard_.get()));

    rootSeq->add_node(std::move(selector));

    root_ = std::move(rootSeq);
}

void EnemyAIController::FixHysteresis()
{
    // 追跡開始 > 停止 を必ず守る（最小マージン）
    const float kMinGap = 0.5f;

    if (chaseStartDist_ <= stopDist_ + kMinGap) {
        chaseStartDist_ = stopDist_ + kMinGap;
    }

    // ついでにマイナス防止
    if (stopDist_ < 0.0f) stopDist_ = 0.0f;
    if (chaseStartDist_ < 0.0f) chaseStartDist_ = 0.0f;
}

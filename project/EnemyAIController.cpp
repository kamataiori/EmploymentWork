#include "EnemyAIController.h"
#include "Enemy/Enemy.h"
#include "MathFunctions.h"

// Enemyにあった角度ラップ補助をここにも用意
static float WrapDeltaRad(float a) {
    while (a > 3.1415926535f) a -= 6.283185307f;
    while (a < -3.1415926535f) a += 6.283185307f;
    return a;
}
static float LerpAngleRad(float from, float to, float t) {
    const float d = WrapDeltaRad(to - from);
    return from + d * t;
}

//======================================================
// Initialize
// ・Enemyと紐付け
// ・BehaviorTreeを構築
//======================================================
void EnemyAIController::Initialize(Enemy* owner)
{
    owner_ = owner;
    BuildTree();
}

//======================================================
// Update
// ・毎フレーム呼ばれる
// ・BTを1ステップ進める
//======================================================
void EnemyAIController::Update(float dt)
{
    if (!owner_) { return; }

    BT::Context ctx{};
    ctx.owner = owner_;
    ctx.dt = dt;

    // BT実行
    tree_.Tick(ctx);
}

//======================================================
// BuildTree
//
// 現在の構成：
//   Root
//    └ Sequence
//       ├ FindTarget
//       └ Selector
//          ├ Sequence（遠い）
//          │   ├ IsTargetFar
//          │   └ Chase
//          └ Near（何もしない）
//
// ※ Near 部分は将来 AttackSelector に差し替える
//======================================================
void EnemyAIController::BuildTree()
{
    using namespace BT;

    auto root = std::make_unique<Sequence>();
    root->SetName("Root");

    root->AddChild(std::make_unique<Condition>(
        [this](Context& ctx) { return FindTarget(ctx); },
        "FindTarget"
    ));

    auto selector = std::make_unique<Selector>();
    selector->SetName("NearOrChase");

    auto chaseSeq = std::make_unique<Sequence>();
    chaseSeq->SetName("ChaseSeq");

    chaseSeq->AddChild(std::make_unique<Condition>(
        [this](Context& ctx) { return IsTargetFar(ctx); },
        "IsTargetFar"
    ));

    chaseSeq->AddChild(std::make_unique<Action>(
        [this](Context& ctx) { return Chase(ctx); },
        "Chase"
    ));

    selector->AddChild(std::move(chaseSeq));

    // 近い時：将来Attackへ置き換えポイント
    selector->AddChild(std::make_unique<Action>(
        [](Context&) { return Status::Success; },
        "Near(IdleOrAttackLater)"
    ));

    root->AddChild(std::move(selector));
    tree_.SetRoot(std::move(root));
}

//======================================================
// FindTarget
// ・Playerなどのターゲットを取得
// ・ctx.target にセットして次ノードへ渡す
//======================================================
bool EnemyAIController::FindTarget(BT::Context& ctx)
{
    const Transform* t = nullptr;

    // Scene側から注入された getter があれば使用
    if (targetGetter_) {
        t = targetGetter_();
    }

    // なければ Enemy が保持している target を使用
    if (!t) {
        t = owner_->GetTargetTransform();
    }

    ctx.target = t;
    return (t != nullptr);
}

//======================================================
// IsTargetFar
// ・距離判定専用
// ・BTでは「条件」だけに責務を限定
//======================================================
bool EnemyAIController::IsTargetFar(BT::Context& ctx) const
{
    auto* enemy = static_cast<Enemy*>(ctx.owner);
    const Transform* target = static_cast<const Transform*>(ctx.target);
    if (!enemy || !target) { return false; }

    Vector3 to = target->translate - enemy->GetTransform().translate;
    to.y = 0.0f;

    float dist = Length(to);
    return dist > chaseStartDist_;
}

//======================================================
// Chase
// - 追跡：向きをターゲットに合わせて、前進
// - stopDist_ より近いなら Success（追跡終了）
// - 基本は Running（追跡継続）
//
// ※「リアルさ」を上げたくなったら：
//   - 加速/減速（速度カーブ）
//   - 回転速度制限
//   - 障害物回避（NavMesh）
//   - “追跡→攻撃→離脱” のコンボをBTで組む
//======================================================
BT::Status EnemyAIController::Chase(BT::Context& ctx)
{
    auto* enemy = static_cast<Enemy*>(ctx.owner);
    const Transform* target = static_cast<const Transform*>(ctx.target);
    if (!enemy || !target) { return BT::Status::Failure; }

    Transform e = enemy->GetTransform();

    Vector3 to = target->translate - e.translate;
    to.y = 0.0f;

    float dist = Length(to);
    if (dist < stopDist_) {
        // 近い：追跡終了（将来 Attack をするならここを Attack に回すのもOK）
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
        return BT::Status::Success;
    }

    // 方向
    Vector3 dir = (dist > 1e-6f) ? Normalize(to) : Vector3{ 0,0,1 };

    // 向き（Yaw）
    float desiredYaw = std::atan2(dir.x, dir.z);
    e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, turnLerp_);

    // 前進（TimeManagerのdtが入っているので dt スケール）
    e.translate.x += dir.x * chaseSpeed_;
    e.translate.z += dir.z * chaseSpeed_;

    enemy->SetTarnsform(e);
    enemy->SetAnimationIfChanged(enemy->GetAnimSet().Run);

    return BT::Status::Running;
}

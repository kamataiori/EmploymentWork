#include "IsTargetFarLeaf.h"
#include "Enemy/Enemy.h"
#include "MathFunctions.h"

IsTargetFarLeaf::IsTargetFarLeaf(BlackBoard* bb, float chaseStartDist)
    : LeafNodeBase(bb), chaseStartDist_(chaseStartDist) {
}

void IsTargetFarLeaf::init()
{
    NodeBase::init();
}

void IsTargetFarLeaf::tick()
{
    // BlackBoard から Enemy 本体を取得
    Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
    // BlackBoard から ターゲット（Player）の Transform を取得
    const Transform* target = mpBlackBoard->get_value<const Transform*>("target");

    // Enemy または Target が存在しない場合は判定不能
    // → Fail として扱い、追跡には進ませない
    if (!enemy || !target) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    // Enemy → Target へのベクトルを計算
    Vector3 to = target->translate - enemy->GetTransform().translate;
    to.y = 0.0f;

    // 水平方向の距離を計算
    float dist = Length(to);
    // 追跡開始距離より遠いかどうかで結果を決定
    // ・Success：追跡を開始してよい
    // ・Fail   ：すでに近い（Idle / 攻撃などに遷移）
    mNodeResult = (dist > chaseStartDist_) ? NodeResult::Success : NodeResult::Fail;
}

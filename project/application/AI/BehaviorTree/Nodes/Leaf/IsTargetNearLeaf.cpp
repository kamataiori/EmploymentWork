#include "IsTargetNearLeaf.h"
#include "Enemy/Enemy.h"
#include "MathFunctions.h"

void IsTargetNearLeaf::tick()
{
    Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
    const Transform* target = mpBlackBoard->get_value<const Transform*>("target");

    if (!enemy || !target) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    Vector3 to = target->translate - enemy->GetTransform().translate;
    to.y = 0.0f; // 平面距離で判定

    const float dist = Length(to);

    // 近いなら攻撃へ
    mNodeResult = (dist <= attackDist_) ? NodeResult::Success : NodeResult::Fail;
}

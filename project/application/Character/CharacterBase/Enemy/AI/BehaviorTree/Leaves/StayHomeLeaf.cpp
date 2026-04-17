#include "StayHomeLeaf.h"
#include "Enemy/Enemy.h"
#include "MathFunctions.h"
#include <cmath>

// 角度差分を [-pi, pi] に折りたたむ
static float WrapDeltaRad(float a) {
    while (a > 3.1415926535f) a -= 6.283185307f;
    while (a < -3.1415926535f) a += 6.283185307f;
    return a;
}
// 角度の線形補間（ラップ考慮）
static float LerpAngleRad(float from, float to, float t) {
    const float d = WrapDeltaRad(to - from);
    return from + d * t;
}

StayHomeLeaf::StayHomeLeaf(BlackBoard* bb, float turnLerp)
    : LeafNodeBase(bb), turnLerp_(turnLerp) {
}

void StayHomeLeaf::init()
{
    NodeBase::init();
}

void StayHomeLeaf::tick()
{
    Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
    if (!enemy) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    // 1) 位置は常に初期位置へ固定
    Transform e = enemy->GetTransform();
    e.translate = enemy->GetHomePosition();

    // 2) ターゲットがいるなら、向きだけターゲットへ向ける（Yawのみ）
    const Transform* target = mpBlackBoard->get_value<const Transform*>("target");
    if (target) {
        Vector3 to = target->translate - e.translate;
        to.y = 0.0f;

        float dist = Length(to);
        if (dist > 1e-6f) {
            Vector3 dir = Normalize(to);
            float desiredYaw = std::atan2(dir.x, dir.z);
            e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, turnLerp_);
        }
    }

    enemy->SetTransform(e);

    // 位置固定ノードとしては常に成功でOK
    mNodeResult = NodeResult::Success;
}
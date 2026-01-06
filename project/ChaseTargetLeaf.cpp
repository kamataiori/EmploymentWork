#include "ChaseTargetLeaf.h"
#include "Enemy/Enemy.h"
#include "MathFunctions.h"
#include <cmath>

// Enemy側の補助と同等（角度差を [-pi, pi] へ）
static float WrapDeltaRad(float a) {
    while (a > 3.1415926535f) a -= 6.283185307f;
    while (a < -3.1415926535f) a += 6.283185307f;
    return a;
}
static float LerpAngleRad(float from, float to, float t) {
    const float d = WrapDeltaRad(to - from);
    return from + d * t;
}

ChaseTargetLeaf::ChaseTargetLeaf(BlackBoard* bb, float stopDist, float chaseSpeed, float turnLerp)
    : LeafNodeBase(bb), stopDist_(stopDist), chaseSpeed_(chaseSpeed), turnLerp_(turnLerp) {
}

void ChaseTargetLeaf::init()
{
    // mNodeResult を Running にする
    NodeBase::init();
}

void ChaseTargetLeaf::tick()
{
    //==================================================
    // 1) BlackBoard から必要な参照を取得
    //==================================================
    // 追跡を行う主体（Enemy）
    Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
    // 追跡対象（Playerなど）の Transform
    const Transform* target = mpBlackBoard->get_value<const Transform*>("target");

    // どちらか欠けていると追跡不能なので Fail
    // （上位ノード側で別行動へ分岐させる）
    if (!enemy || !target) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    //==================================================
    // 2) Enemy の Transform をローカルにコピー
    //==================================================
    // このコピーに対して回転/移動を適用して、最後に SetTarnsform で反映する
    Transform e = enemy->GetTransform();

    //==================================================
    // 3) Enemy → Target 方向ベクトル（平面距離で判定）
    //==================================================
    Vector3 to = target->translate - e.translate;

    // 高さ(Y)は無視して、地面上の追跡として扱う
    // （段差やジャンプで追跡が暴れるのを防ぐ）
    to.y = 0.0f;

    //==================================================
    // 4) 距離が十分近いなら追跡終了（Success）
    //==================================================
    float dist = Length(to);
    if (dist < stopDist_) {
        // 追跡終了なので Idle に戻す（演出的にも自然）
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);

        // このノードは「追跡完了」を上位へ伝える
        mNodeResult = NodeResult::Success;
        return;
    }

    //==================================================
    // 5) 追跡方向（正規化）を作る
    //==================================================
    // dist が極小のとき Normalize が不安定になるので、
    // 念のためフォールバック方向を用意する
    Vector3 dir = (dist > 1e-6f) ? Normalize(to) : Vector3{ 0,0,1 };

    //==================================================
    // 6) 向き（Yaw）をターゲット方向へ滑らかに回す
    //==================================================
    // dir から「目標Yaw角」を計算（X,Zの平面）
    float desiredYaw = std::atan2(dir.x, dir.z);

    // 現在Yaw → 目標Yaw を補間（最短回転で）
    e.rotate.y = LerpAngleRad(e.rotate.y, desiredYaw, turnLerp_);

    //==================================================
    // 7) 前進（追跡移動）
    //==================================================
    // 既存EnemyAIControllerと同じ運用
    // dtを掛けず「1フレームあたりの移動量」として chaseSpeed_ を使う
    // もし TimeManager の dt を使う方針にするなら
    //   chaseSpeed_ を units/sec にして「* dt」を掛けるように変更する
    e.translate.x += dir.x * chaseSpeed_;
    e.translate.z += dir.z * chaseSpeed_;

    // 変更した Transform を Enemy に反映
    enemy->SetTarnsform(e);

    //==================================================
    // 8) 追跡中アニメーション
    //==================================================
    enemy->SetAnimationIfChanged(enemy->GetAnimSet().Run);

    //==================================================
    // 9) まだ追跡継続なので Running を返す
    //==================================================
    mNodeResult = NodeResult::Running;
}

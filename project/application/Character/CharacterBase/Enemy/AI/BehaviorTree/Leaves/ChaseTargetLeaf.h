#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

// 前方宣言
class Enemy;
struct Transform;

//======================================================
// ChaseTargetLeaf
//------------------------------------------------------
// 「ターゲットへ追跡する」行動ノード（Leaf）
//
// 役割
// ・Enemy をターゲット方向へ向かせる（Yaw回転）
// ・ターゲットへ近づく（前進）
// ・追跡中は Run アニメ
// ・十分近づいたら Idle にして追跡終了
//
// NodeResult の意味：
// ・Running : 追跡継続中（まだ遠い）
// ・Success : 追跡完了（stopDist_ より近づいた）
// ・Fail    : 追跡不能（enemy/target が取れない等）
//
// BlackBoard から読むキー：
//   "enemy"  : Enemy*            … 操作対象の敵
//   "target" : const Transform*  … 追跡対象（player等）のTransform
//======================================================
class ChaseTargetLeaf : public LeafNodeBase {
public:

    //==================================================
    // コンストラクタ
    //--------------------------------------------------
    // stopDist    : この距離より近づいたら追跡終了（Success）
    // chaseSpeed  : 1フレームあたりの移動量（※dtを掛けない設計なら“1F速度”）
    // turnLerp    : 向きの補間率（0に近いほどゆっくり旋回、1で即時向き）
    //==================================================
    ChaseTargetLeaf(BlackBoard* bb, float stopDist, float chaseSpeed, float turnLerp);

protected:
    void init() override;
    void tick() override;

private:
    // この距離未満になったら Success を返して追跡を終える
    float stopDist_ = 3.0f;

    // 追跡速度（1フレームの移動量）
    // dtを掛けるなら「units/sec」になる
    // dtを掛けないなら「units/frame」になる（現状は後者運用）
    float chaseSpeed_ = 0.22f;

    // 旋回の滑らかさ（Yaw補間率）
    // 0.0f だと一切回らない
    // 1.0f だと瞬時にターゲット方向へ向く
    float turnLerp_ = 0.18f;
};

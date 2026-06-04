#pragma once
#include "EnemyActionState.h"
#include "application/Character/CharacterBase/Enemy/AI/JumpSlamParam.h"
#include "MathFunctions.h"

//======================================================
// JumpSlamState
//------------------------------------------------------
// ジャンプ急降下叩きつけ（間合いを詰める追撃技）
//   WindUp(溜め) → Leap(放物線跳躍) → Impact(着地衝撃) → Recover(硬直)
//
// ・回転薙ぎ払いが「近距離を罰する」のに対し、こちらは
//   距離を取って逃げたプレイヤーを追って間合いを詰める
// ・溜め中に確定した着地点へ跳ぶため、移動して回避できる
// ・着地点に範囲攻撃判定とカメラ振動を発生させる
//======================================================
class JumpSlamState : public EnemyActionState
{
public:
    explicit JumpSlamState(const JumpSlamParam& param);

    void Enter(Enemy* enemy) override;
    bool Update(Enemy* enemy, float dt) override;
    void Exit(Enemy* enemy) override;

    const char* GetName() const override { return "JumpSlam"; }

private:
    enum class Phase {
        WindUp,   // 溜め（しゃがみ込み）
        Leap,     // 放物線跳躍
        Impact,   // 着地衝撃（判定あり）
        Recover,  // 硬直
    };

    Phase phase_ = Phase::WindUp;
    JumpSlamParam param_;

    float timer_ = 0.0f;
    float baseY_ = 0.0f;        // 開始時の接地Y

    Vector3 leapStart_{};       // 跳躍の始点
    Vector3 leapTarget_{};      // 跳躍の着地点（溜め終わりに確定）
};

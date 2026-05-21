#pragma once
#include "EnemyActionState.h"
#include "application/Character/CharacterBase/Enemy/AI/FeintAttackParam.h"
#include "MathFunctions.h"

//======================================================
// FeintAttackState
//------------------------------------------------------
// フェイント攻撃（読み合い技）
//   WindUp(偽の溜め) → FeintHold(誘い) → Lunge(本攻撃) → Recover(硬直)
//
// ・WindUp は回転薙ぎ払いと同じ構え。プレイヤーは攻撃を予測する
// ・FeintHold で動かず静止し、プレイヤーの「早すぎる回避」を誘う
// ・誘いが終わった瞬間、短い溜めで前方へ突き込む（本攻撃）
//   → 早回避したプレイヤーは硬直中に突きを食らう
// ・じっと待ったプレイヤーは突きを見てから回避できる（公平）
//======================================================
class FeintAttackState : public EnemyActionState
{
public:
    explicit FeintAttackState(const FeintAttackParam& param);

    void Enter(Enemy* enemy) override;
    bool Update(Enemy* enemy, float dt) override;
    void Exit(Enemy* enemy) override;

    const char* GetName() const override { return "FeintAttack"; }

private:
    enum class Phase {
        WindUp,     // 偽の予備動作
        FeintHold,  // 構えたまま静止（誘い）
        Lunge,      // 本攻撃（前方へ突き込み）
        Recover,    // 硬直
    };

    Phase phase_ = Phase::WindUp;
    FeintAttackParam param_;

    float timer_ = 0.0f;

    Vector3 lungeStart_{};            // 突き込みの始点
    Vector3 lungeDir_{ 0.0f, 0.0f, 1.0f }; // 突き込みの方向
};

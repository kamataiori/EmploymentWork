#pragma once
#include "EnemyActionState.h"
#include "application/Character/CharacterBase/Enemy/AI/SpinAttackParam.h"

//======================================================
// SpinAttackState
//------------------------------------------------------
// 回転薙ぎ払い（近接撃退技）
//   WindUp(溜め) → Spin(回転攻撃) → Recover(硬直) → 完了
//
// ・WindUp 中にプレイヤーへ向き直り、武器を構えて「離れろ」と予告する
// ・Spin 中はボス周囲に攻撃判定（EnemyAreaAttack）を展開し、
//   その場で高速回転する。張り付いたプレイヤーを罰する
// ・Recover はプレイヤーの反撃チャンス
//======================================================
class SpinAttackState : public EnemyActionState
{
public:
    explicit SpinAttackState(const SpinAttackParam& param);

    void Enter(Enemy* enemy) override;
    bool Update(Enemy* enemy, float dt) override;
    void Exit(Enemy* enemy) override;

    const char* GetName() const override { return "SpinAttack"; }

private:
    enum class Phase {
        WindUp,   // 溜め（予備動作）
        Spin,     // 回転攻撃（判定あり）
        Recover,  // 硬直
    };

    Phase phase_ = Phase::WindUp;
    SpinAttackParam param_;

    float timer_ = 0.0f;
    float baseY_ = 0.0f;  // 開始時の接地Y。ホップはここを基準に上下する
};

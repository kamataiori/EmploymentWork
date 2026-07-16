#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// BattleState
//------------------------------------------------------
// ボス戦。世界を回して当たり判定を取り、決着がついたら演出へ渡すだけ。
// 「どう戦うか」は敵のBT/EnemyStateManager が持っているので、ここは薄い。
//
// 将来コア戦を入れるときは、この手前に CoreBattleState を挟み、
// 全コア破壊で BossAppearState → この BattleState へ、という並びになる。
//======================================================
class BattleState : public BattlePhaseState
{
public:
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "Battle"; }
};

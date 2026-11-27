#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// CoreBattleState（中央コア戦）
//------------------------------------------------------
// Sentinel 全滅後。中央のボスのコアが破壊可能になり、これを壊す局面。
// 破壊したらボス登場（BossAppearState）へ渡す。
//======================================================
class CoreBattleState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "CoreBattle"; }
};

#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// SentinelBattleState（前哨戦）
//------------------------------------------------------
// 四隅の Sentinel が下から迫り上がって登場し、プレイヤーがこれを全滅させる局面。
// ボスはまだ休眠。全滅したら中央コア戦（CoreBattleState）へ渡す。
//======================================================
class SentinelBattleState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "SentinelBattle"; }
};

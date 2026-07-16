#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// SwarmWaveState（第1波：群れ突進の雑魚）
//------------------------------------------------------
// イントロ直後の局面。群れの雑魚が波状に登場し、全波を殲滅すると
// 前哨戦（SentinelBattleState）へ渡す。ボスも Sentinel もまだ眠っている。
//======================================================
class SwarmWaveState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "SwarmWave"; }
};

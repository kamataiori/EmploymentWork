#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// BossAppearState（ボス登場）
//------------------------------------------------------
// 中央コア破壊後。休眠していたボスを起こして定位置の真上から落とし、
// 着地を待って短い間を置いてから本戦（BattleState）へ渡す。
//======================================================
class BossAppearState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "BossAppear"; }

private:
    float appearTimer_ = 0.0f;
    static constexpr float kAppearSeconds_ = 1.2f; // 着地してからの間（この後 BattleState へ）
};

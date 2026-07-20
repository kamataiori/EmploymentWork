#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// SwarmWaveState（第1波：群れ突進の雑魚）
//------------------------------------------------------
// イントロ直後の局面。群れの雑魚が波状に登場し、全波を殲滅すると
// 出現カットシーン（SentinelRevealState）へ渡す。ボスも Sentinel もまだ眠っている。
//
// 入ってすぐには湧かせない。カウントダウン明けに即殴られると理不尽なので、
// 少し間を置いてから第1波を出す。
//======================================================
class SwarmWaveState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "SwarmWave"; }

private:
    float startDelayTimer_ = 0.0f;
    bool  begun_ = false;   // 第1波を出したか

    // カウントダウンが終わってから第1波が湧くまでの間（秒）
    static constexpr float kStartDelay_ = 1.5f;
};

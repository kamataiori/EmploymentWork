#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// LoseState
//------------------------------------------------------
// プレイヤー死亡の決着演出。ビネットを徐々に締めて画面を暗転させ、
// 閉じ切ったらタイトルへ戻す。
//
// プレイヤーが倒れている間もボスは動き続けるので、ここでもワールドと
// 当たり判定は回し続ける。
//======================================================
class LoseState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "Lose"; }

private:
    // ビネット（画面周辺を暗く落とす）
    static constexpr float kVignetteDuration = 2.0f;   // 閉じ切るまでの秒数
    static constexpr float kVignetteScale    = 0.3f;
    static constexpr float kVignettePower    = 3.0f;   // 閉じ切ったときの強さ

    float vignetteTimer_ = 0.0f;
};

#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// WinState
//------------------------------------------------------
// ボス撃破の決着演出。
//   Enter  : カメラをプレイヤー中心にオービットさせ、時間をスローにする
//   Update : 少し待ってからズームイン。ズームが終わったら時間を戻す
//
// この間もワールドと当たり判定は動かし続ける（ボスの死亡アニメ・爆散が進む）。
// タイトルへ戻すのは Enemy 側の死亡タイマーが行う。
//======================================================
class WinState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "Win"; }

private:
    // カメラのオービット（プレイヤーを中心に回り込む）
    static constexpr float kOrbitAngleRatio = 1.5f;   // pi / この値 だけ回す
    static constexpr float kOrbitDuration   = 0.6f;

    // 時間演出
    static constexpr float kSlowMotionScale = 0.1f;

    // ズームイン（オービットが落ち着いてから入る）
    static constexpr float kZoomDelay     = 0.9f;
    static constexpr float kZoomDuration  = 0.5f;
    static constexpr float kZoomFovRatio  = 8.0f;     // pi / この値 が寄り切ったときの画角

    float zoomDelayTimer_ = kZoomDelay;
    float zoomTimer_      = 0.0f;
    bool  zoomStarted_    = false;
};

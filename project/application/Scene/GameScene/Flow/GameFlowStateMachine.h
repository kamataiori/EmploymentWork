#pragma once
#include <memory>
#include "Flow/GameFlowState.h"

struct GameFlowContext;

//======================================================
// GameFlowStateMachine
//------------------------------------------------------
// 局面（GameFlowState）を1つだけ保持して回す。
//
// ChangeState() は「予約」で、切り替えは次の Update() の先頭で行う。
// State の Update() の途中で自分自身が破棄されるのを防ぐため
// （ChangeState を呼んだ State は、その関数から抜けるまで生きている）。
//
// 使い方（GamePlayScene）:
//   Initialize : flow_.ChangeState(std::make_unique<IntroState>());
//   Update     : flow_.Update(ctx_);
//   各描画      : flow_.UIDraw(ctx_); など
//======================================================
class GameFlowStateMachine
{
public:
    // 次の局面を予約する（切り替えは次の Update() の先頭）
    void ChangeState(std::unique_ptr<GameFlowState> next);

    void Update(GameFlowContext& ctx);

    void ForeGroundDraw(GameFlowContext& ctx);
    void ParticleDraw(GameFlowContext& ctx);
    void UIDraw(GameFlowContext& ctx);

    // デバッグ用：現在の局面名（未設定なら "(none)"）
    const char* GetCurrentStateName() const;

private:
    // 予約されている局面があれば、Exit → Enter して切り替える
    void ApplyPendingState(GameFlowContext& ctx);

    std::unique_ptr<GameFlowState> currentState_;
    std::unique_ptr<GameFlowState> pendingState_;
};

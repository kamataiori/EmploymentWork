#include "Flow/GameFlowStateMachine.h"
#include "Flow/GameFlowContext.h"

void GameFlowStateMachine::ChangeState(std::unique_ptr<GameFlowState> next)
{
    pendingState_ = std::move(next);
}

void GameFlowStateMachine::ApplyPendingState(GameFlowContext& ctx)
{
    if (!pendingState_) return;

    if (currentState_) {
        currentState_->Exit(ctx);
    }
    currentState_ = std::move(pendingState_);
    currentState_->Enter(ctx);
}

void GameFlowStateMachine::Update(GameFlowContext& ctx)
{
    ApplyPendingState(ctx);

    if (currentState_) {
        currentState_->Update(ctx);
    }
}

void GameFlowStateMachine::ForeGroundDraw(GameFlowContext& ctx)
{
    if (currentState_) currentState_->ForeGroundDraw(ctx);
}

void GameFlowStateMachine::ParticleDraw(GameFlowContext& ctx)
{
    if (currentState_) currentState_->ParticleDraw(ctx);
}

void GameFlowStateMachine::UIDraw(GameFlowContext& ctx)
{
    if (currentState_) currentState_->UIDraw(ctx);
}

const char* GameFlowStateMachine::GetCurrentStateName() const
{
    return currentState_ ? currentState_->GetName() : "(none)";
}

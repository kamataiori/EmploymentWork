#include "Flow/States/CoreBattleState.h"
#include "Flow/States/BossAppearState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Sentinel/Sentinel.h"

void CoreBattleState::Enter(GameFlowContext& ctx)
{
    // ここで初めて中央コアを破壊可能にする
    if (ctx.centerCore) {
        ctx.centerCore->SetHittable(true);
    }
}

void CoreBattleState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // コア破壊 → ボス登場へ
    if (ctx.centerCore && ctx.centerCore->IsDead()) {
        ctx.flow->ChangeState(std::make_unique<BossAppearState>());
    }
}

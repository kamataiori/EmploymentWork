#include "Flow/States/SwarmWaveState.h"
#include "Flow/States/SentinelBattleState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Swarm/SwarmController.h"

void SwarmWaveState::Enter(GameFlowContext& ctx)
{
    // 第1波を出して波の進行を開始する
    if (ctx.swarm) {
        ctx.swarm->Begin();
    }
}

void SwarmWaveState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // 全波を殲滅 → 前哨戦（Sentinel）へ
    if (ctx.swarm && ctx.swarm->AllWavesCleared()) {
        ctx.flow->ChangeState(std::make_unique<SentinelBattleState>());
    }
}

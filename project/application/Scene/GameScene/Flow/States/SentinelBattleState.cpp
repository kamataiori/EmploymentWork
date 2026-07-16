#include "Flow/States/SentinelBattleState.h"
#include "Flow/States/CoreBattleState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Sentinel/SentinelField.h"

void SentinelBattleState::Enter(GameFlowContext& ctx)
{
    // 四隅の Sentinel を下から迫り上がらせる
    if (ctx.sentinels) {
        ctx.sentinels->StartSpawn();
    }
}

void SentinelBattleState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北（死亡モーションを見せ切ってから決着演出へ）
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // Sentinel 全滅 → 中央コアが破壊可能になる局面へ
    if (ctx.sentinels && ctx.sentinels->AllDefeated()) {
        ctx.flow->ChangeState(std::make_unique<CoreBattleState>());
    }
}

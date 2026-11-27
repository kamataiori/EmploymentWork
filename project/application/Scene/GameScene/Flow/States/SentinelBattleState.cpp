#include "Flow/States/SentinelBattleState.h"
#include "Flow/States/CoreRevealState.h"
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

    // Sentinel 全滅 → 出現カットシーンへ（カメラをコアへ寄せ→迫り上がりを見せる）
    if (ctx.sentinels && ctx.sentinels->AllDefeated()) {
        ctx.flow->ChangeState(std::make_unique<CoreRevealState>());
    }
}

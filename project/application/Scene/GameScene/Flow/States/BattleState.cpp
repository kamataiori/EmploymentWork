#include "Flow/States/BattleState.h"
#include "Flow/States/WinState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Enemy.h"

void BattleState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北：死亡モーションを見せ切ってから演出へ移る（deathTimer_ は 3秒から減っていく）
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // 勝利：ボス撃破。以降の死亡アニメと退場は WinState 側で回し続ける
    if (ctx.boss->IsDead()) {
        ctx.flow->ChangeState(std::make_unique<WinState>());
    }
}

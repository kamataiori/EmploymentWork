#include "Flow/States/BossAppearState.h"
#include "Flow/States/BattleState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Enemy.h"
#include "engine/TimeManager.h"

void BossAppearState::Enter(GameFlowContext& ctx)
{
    // 休眠していたボスを起こす（ここから Update/描画/当たり判定が動き出す）
    ctx.boss->SetActive(true);
    appearTimer_ = 0.0f;
}

void BossAppearState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // 登場の間を置いてから本戦へ
    appearTimer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();
    if (appearTimer_ >= kAppearSeconds_) {
        ctx.flow->ChangeState(std::make_unique<BattleState>());
    }
}

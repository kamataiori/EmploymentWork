#include "Flow/States/SwarmWaveState.h"
#include "Flow/States/SentinelRevealState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Swarm/SwarmController.h"
#include "engine/UI/MissionBanner.h"
#include "engine/TimeManager.h"

void SwarmWaveState::Enter(GameFlowContext& ctx)
{
    // ここでは湧かせない。カウントダウン明けに即殴られないよう、
    // Update で間を置いてから第1波を出す。
    startDelayTimer_ = 0.0f;
    begun_ = false;

    // 湧く前の間に目的を流す
    if (ctx.missionBanner) {
        ctx.missionBanner->Show(MissionBanner::Swarm);
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

    // 入りの間。空けきったら第1波を出して波の進行を開始する
    if (!begun_) {
        startDelayTimer_ += TimeManager::GetInstance()->GetDeltaTime();
        if (startDelayTimer_ >= kStartDelay_) {
            if (ctx.swarm) {
                ctx.swarm->Begin();
            }
            begun_ = true;
        }
        return; // まだ1体も出していないので、全滅判定へは進ませない
    }

    // 全波を殲滅 → 出現カットシーンへ（カメラを奥へ切替→Sentinel登場を見せる）
    if (ctx.swarm && ctx.swarm->AllWavesCleared()) {
        ctx.flow->ChangeState(std::make_unique<SentinelRevealState>());
    }
}

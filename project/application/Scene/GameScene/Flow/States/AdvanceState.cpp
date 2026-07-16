#include "Flow/States/AdvanceState.h"
#include "Flow/States/SentinelBattleState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Sentinel/SentinelField.h"
#include <cmath>

void AdvanceState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北（保険：この局面では誰も攻撃してこないが、死亡演出中の到達に備える）
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // プレイヤーが奥エリア中心へ十分近づいたら前哨戦を開始する
    const Vector3 p = ctx.player->GetTransform().translate;
    const float dx = p.x - ctx.backArenaCenter.x;
    const float dz = p.z - ctx.backArenaCenter.z;
    const bool arrived = std::sqrt(dx * dx + dz * dz) <= kArrivalRadius;

    // 到達を待たずに Sentinel を全滅させてしまった場合もここを抜ける。
    // Sentinel は SentinelRevealState で既に地上に出ており、しかも四隅は中心から
    // 遠い（halfExtent 32 ＝ 約45）。突進乱舞（V）は対象の位置へ飛び込むので、
    // 中心へ一度も寄らずに4体倒し切れてしまう。到達だけを条件にすると、その場合
    // この局面から抜けられず、中央コアが永久に出てこない。
    const bool sentinelsCleared = ctx.sentinels && ctx.sentinels->AllDefeated();

    if (arrived || sentinelsCleared) {
        ctx.flow->ChangeState(std::make_unique<SentinelBattleState>());
    }
}

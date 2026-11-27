#include "Flow/States/CoreRevealState.h"
#include "Flow/States/CoreBattleState.h"
#include "Flow/States/LoseState.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

#include "Player.h"
#include "Enemy/Sentinel/Sentinel.h"
#include "FollowCamera.h"
#include "engine/UI/MissionBanner.h"
#include "engine/TimeManager.h"

#include <algorithm>

void CoreRevealState::Enter(GameFlowContext& ctx)
{
    timer_ = 0.0f;
    spawnStarted_ = false;

    // 出現はカメラが到着してから（blend-in 完了時に StartSpawn する）。
    // 演出中はプレイヤーを操作させない
    if (ctx.player) {
        ctx.player->SetInputLocked(true);
    }

    // カメラが寄っていく間に、次の目的を流す
    if (ctx.missionBanner) {
        ctx.missionBanner->Show(MissionBanner::Core);
    }
}

void CoreRevealState::Update(GameFlowContext& ctx)
{
    UpdateWorld(ctx);
    RunCollisions(ctx);

    // 敗北（保険：この局面では誰も攻撃してこないが、死亡演出中の到達に備える）
    if (ctx.player->IsDead() && ctx.player->GetDeathTimer() <= 0.0f) {
        if (ctx.followCamera) ctx.followCamera->ClearCinematic();
        ctx.flow->ChangeState(std::make_unique<LoseState>());
        return;
    }

    // 演出タイミングは実時間で進める（ヒットストップ等に左右されない）
    timer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();

    // カメラが到着（blend-in 完了）したら、そこで初めてコアを迫り上がらせる
    if (!spawnStarted_ && timer_ >= kBlendIn) {
        if (ctx.centerCore) ctx.centerCore->StartSpawn();
        spawnStarted_ = true;
    }

    // 追従⇔演出カメラのブレンド率（0=通常追従 / 1=演出カメラ）
    float weight;
    if (timer_ < kBlendIn) {
        weight = timer_ / kBlendIn;                                  // 0→1（寄る）
    }
    else if (timer_ < kBlendIn + kHold) {
        weight = 1.0f;                                               // 到着→出現を見せる
    }
    else {
        weight = 1.0f - (timer_ - (kBlendIn + kHold)) / kBlendOut;   // 1→0（戻る）
    }
    weight = std::clamp(weight, 0.0f, 1.0f);
    // スムーズステップで出入りを柔らかく
    weight = weight * weight * (3.0f - 2.0f * weight);

    // コア（奥アリーナ中心）を、手前(-Z)側の高所から見下ろす構図
    const Vector3 c = ctx.backArenaCenter;
    const Vector3 camPos{ c.x, c.y + kCamHeight, c.z - kCamBackOffset };
    const Vector3 lookAt{ c.x, c.y + kLookHeight, c.z };
    if (ctx.followCamera) {
        ctx.followCamera->SetCinematicBlend(camPos, lookAt, weight);
    }

    // 見せ終わったら追従へ戻し、コア戦へ
    if (timer_ >= kTotal) {
        if (ctx.followCamera) ctx.followCamera->ClearCinematic();
        ctx.flow->ChangeState(std::make_unique<CoreBattleState>());
    }
}

void CoreRevealState::Exit(GameFlowContext& ctx)
{
    // どの経路で抜けても必ず追従へ戻し、プレイヤー操作を解放する
    if (ctx.followCamera) ctx.followCamera->ClearCinematic();
    if (ctx.player)       ctx.player->SetInputLocked(false);
}

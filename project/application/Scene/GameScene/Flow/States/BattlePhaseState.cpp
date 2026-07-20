#include "Flow/States/BattlePhaseState.h"
#include "Flow/GameFlowContext.h"

#include "Player.h"
#include "Enemy/Enemy.h"
#include "Enemy/Sentinel/Sentinel.h"
#include "Enemy/Sentinel/SentinelField.h"
#include "Enemy/Swarm/SwarmController.h"
#include "EnemyDropBullet.h"
#include "EnemySplitBullet.h"
#include "SceneController.h"
#include "SkyBox.h"
#include "Object3d.h"
#include "CollisionManager.h"
#include "engine/UI/UIManager.h"
#include "engine/UI/DamagePopupManager.h"

void BattlePhaseState::UpdateWorld(GameFlowContext& ctx)
{
    ctx.stage->Update();
    ctx.skybox->Update();
    ctx.ground->Update();
    ctx.sky->Update();
    ctx.player->Update();
    ctx.boss->Update();   // 休眠中は Enemy 側で即return する
    if (ctx.swarm) {
        ctx.swarm->Update();
    }
    if (ctx.sentinels) {
        ctx.sentinels->Update();
    }
    if (ctx.centerCore) {
        ctx.centerCore->Update();
    }
}

void BattlePhaseState::RunCollisions(GameFlowContext& ctx)
{
    CollisionManager* cm = ctx.collisionManager;

    // ステージ（押し戻しの受け側・静的）
    if (auto* stageCol = ctx.stage->GetStageCollider()) {
        cm->RegisterCollider(stageCol);
    }

    // プレイヤー（本体・押し戻しプロキシ・武器）
    cm->RegisterCollider(ctx.player->GetMultiCollider());
    if (auto* bodyProxy = ctx.player->GetBodyProxyCollider()) {
        cm->RegisterCollider(bodyProxy);
    }
    if (auto* weapon = ctx.player->GetWeaponCollider()) {
        cm->RegisterCollider(weapon);
    }

    // ボス（起きている間だけ登録する。前哨戦の間は休眠しているので登録しない）
    if (ctx.boss->IsActive()) {
        cm->RegisterCollider(ctx.boss->GetMultiCollider());
        if (auto* bodyProxy = ctx.boss->GetBodyProxyCollider()) {
            cm->RegisterCollider(bodyProxy);
        }

        // 回転薙ぎ払いの攻撃判定（出ている間だけ非nullが返る）
        if (auto* areaAttack = ctx.boss->GetActiveAreaAttackCollider()) {
            cm->RegisterCollider(areaAttack);
        }

        // ボスの弾・配下の雑魚
        for (const auto& bullet : ctx.boss->GetDropBullets()) {
            cm->RegisterCollider(bullet->GetMultiCollider());
        }
        for (const auto& bullet : ctx.boss->GetSplitBullets()) {
            cm->RegisterCollider(bullet->GetMultiCollider());
        }
        for (const auto& minion : ctx.boss->GetMinions()) {
            cm->RegisterCollider(minion->GetMultiCollider());
        }
    }

    // 第1波の群れ
    if (ctx.swarm) {
        ctx.swarm->RegisterColliders(cm);
    }

    // 前哨の敵（BVH押し戻し＋被弾スフィア）
    if (ctx.sentinels) {
        ctx.sentinels->RegisterColliders(cm);
    }

    // 中央のボスのコア（地中待機・破壊後は登録しない）
    if (ctx.centerCore && !ctx.centerCore->IsDead() && !ctx.centerCore->IsHidden()) {
        if (auto* bvh = ctx.centerCore->GetBvhCollider()) {
            cm->RegisterCollider(bvh);
        }
        cm->RegisterCollider(ctx.centerCore->GetMultiCollider());
    }

    // 登録は毎フレーム破棄される（CheckAllCollisions の最後で Reset される）
    cm->CheckAllCollisions();
}

void BattlePhaseState::ForeGroundDraw(GameFlowContext& ctx)
{
    ctx.player->ForeGroundDraw();
    ctx.boss->ForeGroundDraw();
    if (ctx.swarm) {
        ctx.swarm->ForeGroundDraw();
    }
    if (ctx.sentinels) {
        ctx.sentinels->ForeGroundDraw();
    }
    if (ctx.centerCore) {
        ctx.centerCore->ForeGroundDraw();
    }
}

void BattlePhaseState::ParticleDraw(GameFlowContext& ctx)
{
    ctx.player->ParticleDraw();
    ctx.boss->ParticleDraw();
    if (ctx.swarm) {
        ctx.swarm->ParticleDraw();
    }
    if (ctx.sentinels) {
        ctx.sentinels->ParticleDraw();
    }
    if (ctx.centerCore) {
        ctx.centerCore->ParticleDraw();
    }
}

void BattlePhaseState::UIDraw(GameFlowContext& ctx)
{
    if (ctx.damagePopup) {
        ctx.damagePopup->Draw();   // 敵の右上に与ダメージ数値
    }
    ctx.uiManager->Draw();
}

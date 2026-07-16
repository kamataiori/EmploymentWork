#include "BattleTargets.h"

#include "Enemy/Enemy.h"
#include "Enemy/Sentinel/Sentinel.h"
#include "Enemy/Sentinel/SentinelField.h"
#include "Enemy/Swarm/SwarmController.h"

void BattleTargets::Initialize(SwarmController* swarm, SentinelField* sentinels,
                               Sentinel* centerCore, Enemy* boss)
{
    swarm_ = swarm;
    sentinels_ = sentinels;
    centerCore_ = centerCore;
    boss_ = boss;
}

void BattleTargets::CollectAliveTargets(std::vector<ITarget*>& out)
{
    // 第1波の群れ（未出現の個体は SwarmEnemy::IsAlive が false を返す）
    if (swarm_) {
        swarm_->CollectAliveTargets(out);
    }

    // 前哨の敵4体（地中待機中・迫り上がり中は hittable_ が false なので入らない）
    if (sentinels_) {
        sentinels_->CollectAliveTargets(out);
    }

    // 中央のボスのコア（Sentinel 全滅で解禁されるまでは狙わせない）
    if (centerCore_ && !centerCore_->IsDead() && centerCore_->IsHittable()) {
        out.push_back(centerCore_);
    }

    // ボス本体＋配下の雑魚。登場するまでは奥で眠っているので対象に入れない
    //（ここを絞らないと、手前の群れと戦っている最中に奥のボスへ突進してしまう）
    if (boss_ && boss_->IsActive()) {
        boss_->CollectAliveTargets(out);
    }
}

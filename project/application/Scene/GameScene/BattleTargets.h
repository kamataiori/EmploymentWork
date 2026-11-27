#pragma once
#include "ITarget.h"

class Enemy;
class Sentinel;
class SentinelField;
class SwarmController;

//======================================================
// BattleTargets（プレイヤーの攻撃対象の供給元）
//------------------------------------------------------
// V スキル（突進乱舞）などが「今どの敵を狙えるか」を引く窓口。
// バトルは 群れ → 前哨の敵4体 → 中央コア → ボス と進むので、
// その時点で本当に殴れる相手だけを返す。
//
// 局面（State）を見に行かず、各エンティティ自身の状態だけで判断する：
//   ・地中で待機中／迫り上がり中の Sentinel は hittable_ が false
//   ・中央コアは Sentinel 全滅（CoreBattleState）まで hittable_ が false
//   ・ボスは登場（BossAppearState）まで active_ が false
// これで「まだ出ていない敵に突進してしまう」事故を防ぐ。
//
// 参照のみを保持する（所有者は GamePlayScene）。
//======================================================
class BattleTargets : public IEnemyTargetProvider
{
public:
    void Initialize(SwarmController* swarm, SentinelField* sentinels,
                    Sentinel* centerCore, Enemy* boss);

    void CollectAliveTargets(std::vector<ITarget*>& out) override;

    // 前哨（四隅の Sentinel・中央コア）しか居ない局面では突進系スキル（V/Q）を撃たせない
    bool AllowsDashSkills() const override;

private:
    SwarmController* swarm_ = nullptr;
    SentinelField*   sentinels_ = nullptr;
    Sentinel*        centerCore_ = nullptr;
    Enemy*           boss_ = nullptr;
};

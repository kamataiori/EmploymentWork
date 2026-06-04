#pragma once
#include "application/AI/BehaviorTree/Nodes/Leaf/LeafNodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

class Enemy;

//======================================================
// PunchAttackLeaf
// ・近接パンチを行うLeaf
// ・攻撃中は Running（追跡に戻らない）
// ・攻撃が終わったら Success
// ・クールダウン中は Fail（攻撃不可扱い）
//
// BlackBoard
//   "enemy" : Enemy*
//   "dt"    : float（EnemyAIControllerが入れている）
//======================================================
class PunchAttackLeaf : public LeafNodeBase {
public:
    PunchAttackLeaf(BlackBoard* bb, float durationSec, float hitTimeSec, float cooldownSec);

protected:
    void init() override;
    void tick() override;

private:
    // ---- 調整パラメータ ----
    float durationSec_ = 0.60f;   // 攻撃全体の長さ（秒）
    float hitTimeSec_ = 0.15f;   // 当たり判定タイミング（秒）
    float cooldownSec_ = 0.50f;   // 連打防止（秒）

    // ---- 実行時状態 ----
    float timer_ = 0.0f;          // 攻撃開始からの経過
    float cooldownTimer_ = 0.0f;  // クールダウン残り
    bool started_ = false;        // 攻撃開始したか
    bool hitIssued_ = false;      // 当たり判定を出したか
};

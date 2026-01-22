#include "PunchAttackLeaf.h"
#include "Enemy/Enemy.h"

PunchAttackLeaf::PunchAttackLeaf(BlackBoard* bb, float durationSec, float hitTimeSec, float cooldownSec)
    : LeafNodeBase(bb),
    durationSec_(durationSec),
    hitTimeSec_(hitTimeSec),
    cooldownSec_(cooldownSec)
{
}

void PunchAttackLeaf::init()
{
    NodeBase::init();
    timer_ = 0.0f;
    started_ = false;
    hitIssued_ = false;
}

void PunchAttackLeaf::tick()
{
    Enemy* enemy = mpBlackBoard->get_value<Enemy*>("enemy");
    if (!enemy) {
        mNodeResult = NodeResult::Fail;
        return;
    }

    float dt = TimeManager::GetInstance()->GetDeltaTime();

    //==================================================
    // 0) クールダウン中は攻撃できない
    //==================================================
    if (cooldownTimer_ > 0.0f) {
        cooldownTimer_ -= dt;
        mNodeResult = NodeResult::Fail; // 上位Selectorが別行動へ
        return;
    }

    //==================================================
    // 1) 攻撃開始（最初の1回だけ）
    //==================================================
    if (!started_) {
        started_ = true;

        // Enemyのアニメ名セットに Punch がある（最新Enemy.h確認済）
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Punch);

        // ※ 攻撃中は Chase を走らせたくないので Running 拘束する
        // ※ ここで「向きをターゲットに合わせる」なども拡張可能
    }

    //==================================================
    // 2) 経過時間を進める
    //==================================================
    timer_ += dt;

    //==================================================
    // 3) 当たり判定のタイミング（1回だけ）
    //==================================================
    if (!hitIssued_ && timer_ >= hitTimeSec_) {
        hitIssued_ = true;

        // ここで「ヒットボックス生成」や「プレイヤーへダメージ」へ接続する
        // 現状のEnemy.cppには“相手へ与ダメ”の関数がまだ無いので、まずはHook推奨：
        // enemy->RequestPunchHit();
    }

    //==================================================
    // 4) 攻撃終了
    //==================================================
    if (timer_ >= durationSec_) {
        enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
        cooldownTimer_ = cooldownSec_;
        mNodeResult = NodeResult::Success;
        return;
    }

    // 攻撃中は拘束（追跡などに遷移しない）
    mNodeResult = NodeResult::Running;
}

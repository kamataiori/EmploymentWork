#include "PlayerRogue.h"

void PlayerRogue::Initialize()
{
    // 基底クラスの初期化
    PlayerBase::Initialize();

    // アニメーションの名前をセット
    SetAnimationNames();
}

void PlayerRogue::Update()
{
    // 基底クラスの共通更新処理
    PlayerBase::Update();
}

void PlayerRogue::SetAnimationNames()
{
    // Rogueの実アニメ名（Blender名）
    rogueAnim_.Idle = "Idle";
    rogueAnim_.Run = "Run";
    rogueAnim_.Attacking_Idle = "Attacking_Idle"; // あるなら
    // 必要に応じて他もここでセット
    // rogueAnim_.Dagger_Attack  = "Dagger_Attack";
    // rogueAnim_.Dagger_Attack2 = "Dagger_Attack2";
    // ...

    // 共通キー用（AnimationSet）に写す
    baseAnim_.Idle = rogueAnim_.Idle;
    baseAnim_.Run = rogueAnim_.Run;
    baseAnim_.Run_Weapon = rogueAnim_.Run; // RogueにRun_Weaponが無いのでRunで代用
    baseAnim_.Idle_Attacking = rogueAnim_.Attacking_Idle; // あれば

    // 他キーが必要なら適宜マッピングを追加
    // baseAnim_.Walk = rogueAnim_.Walk; など
}


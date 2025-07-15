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
    // アニメーションの名前をセット
    animation_.Idle = "Idle";
    animation_.Run = "Run";

    // 共通用マッピング
    baseAnim_.Idle = animation_.Idle;
    baseAnim_.Run = animation_.Run;
}


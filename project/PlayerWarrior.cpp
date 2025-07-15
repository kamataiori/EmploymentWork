#include "PlayerWarrior.h"

void PlayerWarrior::Initialize()
{
	// 基底クラスの初期化
	PlayerBase::Initialize();

	// アニメーションの名前をセット
	animation_.Idle = "Idle";
	animation_.Idle_Weapon = "Idle_Weapon";
	animation_.Run_Weapon = "Run_Weapon";
}

void PlayerWarrior::Update()
{
	// 基底クラスの共通更新処理
	PlayerBase::Update();
}

void PlayerWarrior::SetAnimationNames()
{
	// アニメーションの名前をセット
	animation_.Idle = "Idle";
	animation_.Idle_Weapon = "Idle_Weapon";
	animation_.Run_Weapon = "Run_Weapon";
}

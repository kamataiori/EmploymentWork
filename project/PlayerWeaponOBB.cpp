#include "PlayerWeaponOBB.h"
#include <Input.h>
#include <PlayerAnimKey.h>
#include "PlayerBase.h"

void PlayerWeaponOBB::Initialize()
{
	

}

void PlayerWeaponOBB::Update()
{
	// コライダー位置を更新
	//SetPosition(playerTransform_->translate);

	/*if (playerTransform_) {
		SetPosition(playerTransform_->translate);
	}*/

	//obb.color = static_cast<int>(Color::WHITE);
}

void PlayerWeaponOBB::Draw()
{
	//OBBCollider::Draw();
}

void PlayerWeaponOBB::NormalAttack()
{
	const bool now = Input::GetInstance()->PushKey(DIK_E);
	if (now && !IsnormalAttack_) {                  // 立ち上がり(エッジ)だけ
		if (owner_) {
			// 優先度10, 再生ロック0.8秒
			owner_->RequestAnimKey(PlayerAnimKey::SwordAttackFast, 10, 0.8f);
		}
	}
	IsnormalAttack_ = now;
}

void PlayerWeaponOBB::Skill()
{
	const bool nowF = Input::GetInstance()->PushKey(DIK_F);
	if (nowF && !IsSkill_) {                  // 立ち上がりだけ
		if (owner_) {
			owner_->RequestAnimKey(PlayerAnimKey::Roll, 10, 0.8f);
		}
	}
	IsSkill_ = nowF;
}

void PlayerWeaponOBB::Ultimate()
{
}

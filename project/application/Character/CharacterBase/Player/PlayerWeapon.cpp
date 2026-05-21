#include "PlayerWeapon.h"
#include <Input.h>
#include "Player.h"

//======================================================
// ノーマル攻撃3段のテーブル
//   key      : 再生アニメ
//   speed    : 再生速度倍率（1.0=等倍 / >1速い / <1遅い）
//   hitStart : 当たり判定ON  （アニメ進行度 0〜1）
//   hitEnd   : 当たり判定OFF （アニメ進行度 0〜1）
//   comboAt  : この進行度を超えたら次段へ繋ぐ
// 速度を変えても hit/combo は進行度基準なので自動で同期する。
//======================================================
const PlayerWeapon::AttackInfo PlayerWeapon::kNormalAttacks_[3] = {
	//  key,                      speed, hitStart, hitEnd, comboAt
	{ PlayerAnimKey::Attack01,     1.2f,    0.25f,  0.45f,  0.60f }, // 速い小振り
	{ PlayerAnimKey::Attack02,     1.0f,    0.30f,  0.55f,  0.65f }, // 標準
	{ PlayerAnimKey::Attack03,     0.8f,    0.40f,  0.70f,  0.80f }, // ゆっくり大振り
};

void PlayerWeapon::Initialize()
{

}

void PlayerWeapon::StartAttack(int index)
{
	if (index < 0 || index > 2) return;

	activeAttackIndex_ = index;
	attacking_ = true;
	comboReserve_ = false;

	if (owner_) {
		const AttackInfo& a = kNormalAttacks_[index];
		// lockSec は使わない（攻撃の継続は進行度で管理する）。速度倍率を渡す。
		owner_->RequestAnimaKey(a.key, 10, 0.0f, a.speed);

		// ズーム演出
		if (auto* effect = owner_->GetCameraEffect()) {
			effect->StartZoomPunch(0.04f, 0.06f, 0.18f);
		}
	}
}

void PlayerWeapon::Update()
{
	if (!attacking_ || !owner_) return;

	// アニメ進行度（再生速度を変えても 0〜1 で進む）
	const float progress = owner_->GetAnimationProgress();
	const AttackInfo& cur = kNormalAttacks_[activeAttackIndex_];

	// ===== コンボ予約があり、受付進行度を超えたら次段へ =====
	if (comboReserve_ && activeAttackIndex_ < 2 && progress >= cur.comboAt) {
		StartAttack(activeAttackIndex_ + 1);
		return;
	}

	// ===== 攻撃アニメが終端まで進んだら攻撃終了 =====
	if (progress >= kAttackEndProgress_) {
		attacking_ = false;
		comboReserve_ = false;
		owner_->EndAttackState(); // 優先度を解除して移動アニメへ戻れるようにする
	}
}


void PlayerWeapon::Draw()
{

}

void PlayerWeapon::NormalAttack()
{
	Input* input = Input::GetInstance();

	// ===== 攻撃中：長押しでも次段を予約できる =====
	if (attacking_) {
		if (input->PushMouseButton(0)) {
			comboReserve_ = true; // 押されてる間ずっと予約ON
		}
		return;
	}

	// ===== 攻撃してない：開始は「押した瞬間」だけ =====
	if (!input->TriggerMouseButton(0)) {
		return;
	}

	// 新規コンボは必ず1段目から
	StartAttack(0);
}


bool PlayerWeapon::IsHitActive() const
{
	if (!attacking_ || !owner_) return false;

	// 当たり判定は進行度ウィンドウ内だけ有効。
	// 振りの当たりフレーム帯（hitStart〜hitEnd）に合わせて調整する。
	const float progress = owner_->GetAnimationProgress();
	const AttackInfo& a = kNormalAttacks_[activeAttackIndex_];
	return (progress >= a.hitStart && progress <= a.hitEnd);
}


void PlayerWeapon::Skill()
{
	const bool nowF = Input::GetInstance()->PushKey(DIK_E);
	if (nowF && !IsSkill_) {                  // 立ち上がりだけ
		if (owner_) {
			// 回避に入るので攻撃状態は打ち切る（当たり判定も消える）
			attacking_ = false;
			comboReserve_ = false;
			owner_->RequestAnimaKey(PlayerAnimKey::Roll, 10, 0.8f);
		}
	}
	IsSkill_ = nowF;
}

void PlayerWeapon::Ultimate()
{

}

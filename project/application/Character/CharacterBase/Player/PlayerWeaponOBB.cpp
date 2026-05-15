#include "PlayerWeaponOBB.h"
#include <Input.h>
#include <PlayerAnimKey.h>
#include "Player.h"
#include "engine/TimeManager.h"

void PlayerWeaponOBB::Initialize()
{
	
}

void PlayerWeaponOBB::Update()
{
	// 前フレームまで攻撃中だったか
	wasActive_ = (activeTime_ > 0.0f);

	// アクティブ時間の減衰
	if (activeTime_ > 0.0f) {
		float dt = TimeManager::GetInstance()->GetDeltaTime();
		activeTime_ -= dt;
		if (activeTime_ < 0.0f) activeTime_ = 0.0f;
	}

	// ===== 攻撃が終わった瞬間に予約があれば次段を発動 =====
	// 攻撃中に、終わる直前 or 終わった瞬間に次段へ切り替える
	if (comboReserve_) {

		// 早めに繋ぐ条件：残りが少ない
		const bool nearEnd = (activeTime_ > 0.0f && activeTime_ <= comboLeadTime_);

		// 万が一 0 まで落ちた場合の救済：直前は攻撃中だった
		const bool justEnded = (wasActive_ && activeTime_ <= 0.0f);

		if (nearEnd || justEnded) {

			comboReserve_ = false;

			// 次段開始（時間をシンプルにリセット）
			activeTime_ = activeDuration_;

			if (owner_) {
				static const PlayerAnimKey comboKeys[3] = {
					PlayerAnimKey::Attack01,
					PlayerAnimKey::Attack02,
					PlayerAnimKey::Attack03
				};
				owner_->RequestAnimaKey(comboKeys[normalComboIndex_], 10, activeDuration_);

				// ズーム演出
				if (auto* effect = owner_->GetCameraEffect()) {
					effect->StartZoomPunch(0.04f, 0.06f, 0.18f);
				}
			}

			normalComboIndex_ = (normalComboIndex_ + 1) % 3;
		}
	}

	// 予約がないまま攻撃が終わったら、次は01から
	if (wasActive_ && activeTime_ <= 0.0f && !comboReserve_) {
		normalComboIndex_ = 0;
	}

}


void PlayerWeaponOBB::Draw()
{

	// 弾描画
	/*for (auto& b : bullets_) {
		b->Draw();
	}*/
}

void PlayerWeaponOBB::NormalAttack()
{
	Input* input = Input::GetInstance();

	// ===== 攻撃中：長押しでも次段を予約できる =====
	if (activeTime_ > 0.0f) {
		if (input->PushMouseButton(0)) {
			comboReserve_ = true; // 押されてる間ずっと予約ON
		}
		return;
	}

	// ===== 攻撃してない：開始は「押した瞬間」だけ =====
	if (!input->TriggerMouseButton(0)) {
		return;
	}

	// この段を開始（段開始時に予約をリセット）
	comboReserve_ = false;
	activeTime_ = activeDuration_;

	if (owner_) {
		static const PlayerAnimKey comboKeys[3] = {
			PlayerAnimKey::Attack01,
			PlayerAnimKey::Attack02,
			PlayerAnimKey::Attack03
		};
		owner_->RequestAnimaKey(comboKeys[normalComboIndex_], 10, activeDuration_);

		// ズーム演出
		if (auto* effect = owner_->GetCameraEffect()) {
			effect->StartZoomPunch(0.04f, 0.06f, 0.18f);
		}
	}

	// 次の段へ
	normalComboIndex_ = (normalComboIndex_ + 1) % 3;
}


void PlayerWeaponOBB::Skill()
{
	const bool nowF = Input::GetInstance()->PushKey(DIK_E);
	if (nowF && !IsSkill_) {                  // 立ち上がりだけ
		if (owner_) {
			owner_->RequestAnimaKey(PlayerAnimKey::Roll, 10, 0.8f);
		}
	}
	IsSkill_ = nowF;
}

void PlayerWeaponOBB::Ultimate()
{
	
}

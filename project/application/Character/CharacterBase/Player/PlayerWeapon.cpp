#include "PlayerWeapon.h"
#include <Input.h>
#include "Player.h"
#include "Sword.h"
#include "SpinSlashSkill.h"
#include "engine/TimeManager.h"

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
	{ PlayerAnimKey::Attack01,     1.3f,    0.25f,  0.45f,  0.60f }, // 速い小振り
	{ PlayerAnimKey::Attack02,     1.15f,    0.30f,  0.55f,  0.65f }, // 少し速い小振り
	{ PlayerAnimKey::Attack03,     0.9f,    0.40f,  0.70f,  0.80f }, // 少しゆっくり大振り
};

void PlayerWeapon::Initialize()
{
	// E キーのスキル（回転斬り）を生成し、依存（プレイヤー・剣）を注入する。
	// IPlayerSkill で保持するので、スキル追加時はここに別クラスを足すだけでよい。
	auto spinSlash = std::make_unique<SpinSlashSkill>();
	spinSlash->Initialize(owner_, sword_);
	eSkill_ = std::move(spinSlash);
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

		// ズーム演出（攻撃時の一瞬寄って戻るパンチズーム）
		if (auto* effect = owner_->GetCameraEffect()) {
			// Attack01 / Attack02 の寄り幅（従来どおり）
			constexpr float kDefaultZoomAmount = 0.04f;

			if (a.key == PlayerAnimKey::Attack03) {
				// Attack03 専用演出：「最初はゆっくりズーム → 一気にぐんっと寄る → 素早く戻す」
				//   溜め … 控えめな量を Linear でじわっと寄せて助走をつける
				//   寄り … 残りを一気に EaseOutExpo で「ぐんっ」と寄せる
				//   戻り … EaseOutCubic で素早く戻す
				constexpr float kAttack03ZoomAmount = 0.10f;  // 最終的な寄り幅
				constexpr float kAttack03PreZoomAmount = 0.025f; // 溜めで先に寄る量
				constexpr float kAttack03PreDuration = 0.50f;  // 溜めの時間（ゆっくり）
				constexpr float kAttack03InDuration = 0.08f;  // 一気に寄る時間
				constexpr float kAttack03OutDuration = 0.18f;  // 戻る時間

				CameraEffectController::ZoomPunchParams punch{};
				punch.zoomAmount = kAttack03ZoomAmount;
				punch.preZoomAmount = kAttack03PreZoomAmount;
				punch.preDuration = kAttack03PreDuration;
				punch.inDuration = kAttack03InDuration;
				punch.outDuration = kAttack03OutDuration;
				punch.preEasing = Tween::Easing::Linear;       // 溜め
				punch.inEasing = Tween::Easing::EaseOutExpo;  // 寄り
				punch.outEasing = Tween::Easing::EaseOutCubic; // 戻り
				effect->StartZoomPunch(punch);
			}
			else {
				// Attack01 / Attack02：従来どおりの素直なパンチズーム
				constexpr float kDefaultInDuration = 0.06f;
				constexpr float kDefaultOutDuration = 0.18f;
				effect->StartZoomPunch(kDefaultZoomAmount,
					kDefaultInDuration, kDefaultOutDuration);
			}
		}
	}
}

void PlayerWeapon::Update()
{
	if (!owner_) return;

	// ===== スキル発動中はスキルが武器を駆動する =====
	// （剣のワールド配置・当たり判定はスキル側で行う。通常攻撃の処理は止める）
	if (IsAnySkillActive()) {
		eSkill_->Update(TimeManager::GetInstance()->GetDeltaTime());
		return;
	}

	// ===== 通常攻撃の進行（コンボ／終了） =====
	if (attacking_) {
		// アニメ進行度（再生速度を変えても 0〜1 で進む）
		const float progress = owner_->GetAnimationProgress();
		const AttackInfo& cur = kNormalAttacks_[activeAttackIndex_];

		// コンボ予約があり、受付進行度を超えたら次段へ
		if (comboReserve_ && activeAttackIndex_ < 2 && progress >= cur.comboAt) {
			StartAttack(activeAttackIndex_ + 1);
		}
		// 攻撃アニメが終端まで進んだら攻撃終了
		else if (progress >= kAttackEndProgress_) {
			attacking_ = false;
			comboReserve_ = false;
			owner_->EndAttackState(); // 優先度を解除して移動アニメへ戻れるようにする
		}
	}

	// 通常攻撃の当たり判定ウィンドウを剣へ反映（非攻撃時は false ＝判定なし）
	if (sword_) sword_->SetHitEnabled(IsHitActive());
}


void PlayerWeapon::Draw()
{

}

void PlayerWeapon::NormalAttack()
{
	// スキル中は通常攻撃を出せない
	if (IsAnySkillActive()) return;

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
	const bool nowE = Input::GetInstance()->PushKey(DIK_E);
	const bool triggered = (nowE && !IsSkill_); // 押した瞬間(立ち上がり)だけ
	IsSkill_ = nowE;

	if (!triggered) return;
	if (IsAnySkillActive()) return; // 多重発動はしない
	if (!eSkill_) return;

	// 進行中の通常攻撃は打ち切ってからスキルへ移行する
	attacking_ = false;
	comboReserve_ = false;

	eSkill_->Start();
}

void PlayerWeapon::Ultimate()
{
	// スキル中は発動しない
	if (IsAnySkillActive()) return;
}

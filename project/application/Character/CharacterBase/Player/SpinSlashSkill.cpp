#include "SpinSlashSkill.h"
#include "Player.h"
#include "Sword.h"

#include <cmath>
#include <numbers>
#include <algorithm>

void SpinSlashSkill::Initialize(Player* owner, Sword* sword)
{
	owner_ = owner;
	sword_ = sword;
}

void SpinSlashSkill::Start()
{
	if (!owner_ || !sword_) return;

	active_ = true;
	elapsed_ = 0.0f;

	// 発動した瞬間のプレイヤーの向きを固定する。
	// 弧の「方向」はこの値で決め打ちにし、スキル中に向きを変えても弧は振り回されない。
	// （位置は毎フレーム追従するので、移動しても手元へ戻る）
	fixedYaw_ = owner_->GetTransform().rotate.y;

	// 剣を手から切り離し、大きめの当たり判定を常時ONにする
	sword_->Detach();
	sword_->SetHitHalfSize(kHitHalfSize_);
	sword_->SetHitEnabled(true);

	// 発射地点（t=0）へ即配置しておく（1フレーム手元に残らないように）
	ApplyArcTransform(0.0f);

	// Attack02 を再生。
	//   priority = kAnimaPriority_(20) … ノーマル攻撃(10)より高く上書き防止
	//   lockSec  = kDuration_          … スキル中は移動アニメで上書きされない
	owner_->RequestAnimaKey(PlayerAnimKey::Attack02,
		kAnimaPriority_, kDuration_, kAnimSpeed_);
}

void SpinSlashSkill::Update(float dt)
{
	if (!active_) return;

	elapsed_ += dt;
	const float t = std::clamp(elapsed_ / kDuration_, 0.0f, 1.0f);

	ApplyArcTransform(t);

	// 終了：剣を手に戻し（当たり判定/サイズも既定へ）、アニメ優先度を解除する
	if (elapsed_ >= kDuration_) {
		active_ = false;
		sword_->Reattach();
		owner_->EndAttackState();
	}
}

void SpinSlashSkill::ApplyArcTransform(float t)
{
	// 位置は毎フレーム参照（プレイヤーが移動しても弧が追従して手元へ戻る）
	const Vector3 playerPos = owner_->GetTransform().translate;

	// 発動時に固定した向きを基準に、左→右へ角度を振る
	const float sweepAngle = kArcStartAngle_ + (kArcEndAngle_ - kArcStartAngle_) * t;
	const float worldAngle = fixedYaw_ + sweepAngle;
	const Vector3 dir = { std::sin(worldAngle), 0.0f, std::cos(worldAngle) };

	// 半径・高さは sin(πt) の山なり：t=0と1で手元(=0)に戻り、t=0.5で最大になる
	const float arcEnvelope = std::sin(std::numbers::pi_v<float> *t); // 0 → 1 → 0
	const float radius = kArcMaxRadius_ * arcEnvelope;
	const float height = kArcBaseHeight_ + kArcLiftHeight_ * arcEnvelope;

	// プレイヤーを中心に弧を描くワールド座標
	const Vector3 worldPos = {
		playerPos.x + dir.x * radius,
		playerPos.y + height,
		playerPos.z + dir.z * radius
	};

	// 自転（ぐるぐる）：横向き(寝た状態)を保ったまま、垂直軸まわりに回す。
	//   Y に弧の向き(worldAngle)＋自転角を加えると、プロペラのように水平回転する。
	const float spinAngle = kArcSpinTurns_ * 2.0f * std::numbers::pi_v<float> *t;
	const Vector3 worldRot = { 0.0f, worldAngle + spinAngle, 0.0f };

	sword_->SetWorldTransform(worldPos, worldRot);
}

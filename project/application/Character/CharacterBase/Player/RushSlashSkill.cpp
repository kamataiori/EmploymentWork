#include "RushSlashSkill.h"
#include "Player.h"
#include "ITarget.h"
#include "MathFunctions.h"
#include "FollowCamera.h"

#include <cmath>
#include <algorithm>

void RushSlashSkill::Initialize(Player* owner)
{
	owner_ = owner;
}

void RushSlashSkill::Start()
{
	if (!owner_ || !provider_) return;

	// 進行中の通常攻撃などを打ち切ってから乱舞へ移行する前提で呼ばれる。
	struck_.clear();
	impactTimer_ = 0.0f;

	// 最初のターゲットはプレイヤー位置から一番近い敵。
	ITarget* first = FindNearestUnstruck(owner_->GetTransform().translate);
	if (!first) {
		// 対象がいなければ何もしない（発動は不成立）。
		active_ = false;
		return;
	}

	active_ = true;

	// 発動中は被弾しない＆通常移動を止める（座標はこのスキルが駆動する）。
	owner_->SetInvincible(true);
	owner_->SetInputLocked(true);

	// 発動中はマウスでカメラを動かせないようにする（突進の演出を固定する）。
	if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
		fc->SetMouseControlEnabled(false);
	}

	BeginDash(first);
}

void RushSlashSkill::Update(float dt)
{
	if (!active_) return;

	switch (phase_) {
	case Phase::Dash:
	{
		// ターゲットが途中で別要因により消えたら、次へ切り替える。
		if (!currentTarget_ || !currentTarget_->IsAlive()) {
			ITarget* next = FindNearestUnstruck(owner_->GetTransform().translate);
			if (next) { BeginDash(next); }
			else { Finish(); }
			return;
		}

		dashTimer_ += dt;

		const Vector3 targetCenter = currentTarget_->GetTargetCenter();

		Transform tf = owner_->GetTransform();

		// XZ 平面でターゲットへ向かう（高さは現状維持＝接地のまま）。
		Vector3 to = targetCenter - tf.translate;
		to.y = 0.0f;
		const float dist = Length(to);

		// 1フレームで進める距離。
		const float step = kDashSpeed_ * dt;

		// 到達判定：このフレームで手前 kStandoffDist_ まで詰められるなら命中とする。
		//   「dist <= kStandoffDist_」だけで判定すると、手前ちょうどで止まった際に
		//   浮動小数点誤差で永遠に成立せず固まることがある。残り距離が1ステップ以内
		//   なら必ず命中させることで、漸近して止まる不具合を防ぐ。
		//   さらに保険として、何らかの理由で到達できない場合もタイムアウトで強制命中する
		//   （“突進は絶対当たる”を担保し、入力ロックが解けないまま固まるのを防ぐ）。
		const bool reached = (dist - kStandoffDist_ <= step) || (dashTimer_ >= kDashTimeout_);
		if (reached) {
			// 命中：大ダメージを直接与える（衝突に頼らないので絶対当たる）。
			currentTarget_->ApplyDamage(kHitDamage_);
			struck_.push_back(currentTarget_);

			// ヒットの一瞬の手応え（軽いズーム）。
			if (auto* effect = owner_->GetCameraEffect()) {
				constexpr float kHitZoomAmount = 0.05f;
				constexpr float kHitZoomInSec = 0.04f;
				constexpr float kHitZoomOutSec = 0.12f;
				effect->StartZoomPunch(kHitZoomAmount, kHitZoomInSec, kHitZoomOutSec);
			}

			phase_ = Phase::Impact;
			impactTimer_ = 0.0f;
			return;
		}

		// 突進移動：向きをターゲットへ合わせつつ前進する。
		// （reached が偽＝dist は kStandoffDist_ + step より大きく、必ず正なので Normalize は安全）
		const Vector3 dir = Normalize(to);
		tf.rotate.y = std::atan2(dir.x, dir.z);
		tf.translate = tf.translate + dir * step;

		owner_->SetTransform(tf);
		break;
	}
	case Phase::Impact:
	{
		impactTimer_ += dt;
		if (impactTimer_ < kImpactDuration_) return;

		// 次は「当たった敵の位置」から一番近い未命中の敵へ。
		const Vector3 fromPos = owner_->GetTransform().translate;
		ITarget* next = FindNearestUnstruck(fromPos);
		if (next) { BeginDash(next); }
		else { Finish(); }
		break;
	}
	}
}

ITarget* RushSlashSkill::FindNearestUnstruck(const Vector3& fromPos)
{
	scratch_.clear();
	provider_->CollectAliveTargets(scratch_);

	ITarget* best = nullptr;
	float bestDistSq = 0.0f;

	for (ITarget* t : scratch_) {
		if (!t || !t->IsAlive()) continue;
		// この発動で既に当てた敵は除外（同じ敵に二度突進しない）。
		if (std::find(struck_.begin(), struck_.end(), t) != struck_.end()) continue;

		const Vector3 d = t->GetTargetCenter() - fromPos;
		const float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
		if (!best || distSq < bestDistSq) {
			best = t;
			bestDistSq = distSq;
		}
	}
	return best;
}

void RushSlashSkill::BeginDash(ITarget* target)
{
	currentTarget_ = target;
	phase_ = Phase::Dash;
	dashTimer_ = 0.0f;

	// 突進斬りモーションを Attack01 / Attack02 で交互に再生する。
	//   struck_.size() は「これまでに命中した数」＝この突進が何体目か(0始まり)。
	//   偶数体目(0,2,4…)=Attack01 / 奇数体目(1,3,5…)=Attack02。
	const PlayerAnimKey animKey =
		(struck_.size() % 2 == 0) ? PlayerAnimKey::Attack01 : PlayerAnimKey::Attack02;

	// 優先度を高くしてロックし、突進中に移動アニメへ落ちないようにする。
	owner_->RequestAnimaKey(animKey, kAnimaPriority_, kAnimLockSec_, kAnimSpeed_);
}

void RushSlashSkill::Finish()
{
	active_ = false;
	currentTarget_ = nullptr;

	// 無敵・入力ロック・アニメ優先度を解除して通常状態へ戻す。
	owner_->SetInvincible(false);
	owner_->SetInputLocked(false);
	owner_->EndAttackState();

	// マウスでのカメラ操作を元に戻す。
	if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
		fc->SetMouseControlEnabled(true);
	}
}

#include "PlayerWeaponOBB.h"
#include <Input.h>
#include <PlayerAnimKey.h>
#include "Player.h"
#include "engine/TimeManager.h"


// Yaw(=Y回転)から OBB の3軸を作るヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	// 右(X), 上(Y), 前(Z)
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f,  0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

// 前方(ローカルZ)へ距離d進めたオフセットをYawで回して足元原点に足す
static Vector3 YawOffset(const Vector3& base, float yaw, const Vector3& localOffset) {
	const float c = std::cos(yaw), s = std::sin(yaw);
	// XZ平面に回転を適用（Yはそのまま）
	Vector3 off{
		localOffset.x * c + localOffset.z * s,
		localOffset.y,
		-localOffset.x * s + localOffset.z * c
	};
	return base + off;
}

static Vector3 MakeFrontCenter(const Vector3& base, float yaw, float frontDist, float height) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	const Vector3 forward = { s, 0.0f, c }; //座標系に合わせ
	return base + forward * frontDist + Vector3{ 0.0f, height, 0.0f };
}

void PlayerWeaponOBB::Initialize()
{
	// 武器専用のMultiColliderを作成（ObjectBaseではないので自前で持つ）
	mc_ = std::make_unique<MultiCollider>();

	// 初期OBBを一つ入れておく
	OBB obb{};
	if (playerTransform_) {
		const float yaw = playerTransform_->rotate.y;
		obb.center = MakeFrontCenter(playerTransform_->translate, yaw, frontDist_, height_);
		Vector3 axes[3]; BuildYawAxes(yaw, axes);
		obb.orientations[0] = axes[0];
		obb.orientations[1] = axes[1];
		obb.orientations[2] = axes[2];
	}
	else {
		obb.center = { 0,0,0 };
		obb.orientations[0] = { 1,0,0 };
		obb.orientations[1] = { 0,1,0 };
		obb.orientations[2] = { 0,0,1 };
	}
	obb.size = { 0.0f, 0.0f, 0.0f }; // 初期は無効

	Shape first{};
	first.kind = ShapeKind::OBB;
	first.obb = obb;

	*mc_ = MultiCollider(first);
	mc_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerWeapon));

	// ヒット時にオーナーへ橋渡し（必要なら）
	if (owner_) {
		mc_->SetHitCallback([this]() {
			owner_->OnCollision();
			});
	}

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

	// ===== ここが追加：攻撃が終わった瞬間に予約があれば次段を発動 =====
	//if (wasActive_ && activeTime_ <= 0.0f) {

	//	if (comboReserve_) {

	//		// 次段開始前に一度リセット（長押しなら次フレームでまたONになる）
	//		comboReserve_ = false;

	//		// 次段を開始
	//		activeTime_ = activeDuration_;

	//		if (owner_) {
	//			static const PlayerAnimKey comboKeys[3] = {
	//				PlayerAnimKey::Attack01,
	//				PlayerAnimKey::Attack02,
	//				PlayerAnimKey::Attack03
	//			};
	//			owner_->RequestAnimaKey(comboKeys[normalComboIndex_], 10, activeDuration_);
	//		}

	//		normalComboIndex_ = (normalComboIndex_ + 1) % 3;
	//	}
	//	else {
	//		normalComboIndex_ = 0;
	//	}
	//}

	// 攻撃中に、終わる直前 or 終わった瞬間に次段へ切り替える（Idle挟み防止）
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
			}

			normalComboIndex_ = (normalComboIndex_ + 1) % 3;
		}
	}

	// 予約がないまま攻撃が終わったら、次は01から
	if (wasActive_ && activeTime_ <= 0.0f && !comboReserve_) {
		normalComboIndex_ = 0;
	}



	// プレイヤー正面に OBB を配置
	if (playerTransform_ && mc_) {
		OBB& obb = mc_->MutableOBB(0);

		const float yaw = playerTransform_->rotate.y;

		obb.center = MakeFrontCenter(playerTransform_->translate, yaw, frontDist_, height_);

		Vector3 axes[3]; BuildYawAxes(yaw, axes);
		obb.orientations[0] = axes[0];
		obb.orientations[1] = axes[1];
		obb.orientations[2] = axes[2];

		obb.size = (activeTime_ > 0.0f) ? obbHalf_ : Vector3{ 0.0f, 0.0f, 0.0f };
	}
}


void PlayerWeaponOBB::Draw()
{
	mc_->Draw();

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

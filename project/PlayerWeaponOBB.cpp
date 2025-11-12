#include "PlayerWeaponOBB.h"
#include <Input.h>
#include <PlayerAnimKey.h>
#include "PlayerBase.h"

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
	const Vector3 forward = { s, 0.0f, c }; // あなたの座標系に合わせてOK
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
	// アクティブ時間の減衰
	if (activeTime_ > 0.0f) {
		activeTime_ -= (1.0f / 60.0f);
		if (activeTime_ < 0.0f) activeTime_ = 0.0f;
	}

	// プレイヤー正面に OBB を配置
	if (playerTransform_ && mc_) {
		OBB& obb = mc_->MutableOBB(0);

		const float yaw = playerTransform_->rotate.y;

		// 中心：足元原点から正面frontDist_、上へheight_
		obb.center = MakeFrontCenter(playerTransform_->translate, yaw, frontDist_, height_);

		// 姿勢：プレイヤーのYawに追従
		Vector3 axes[3]; BuildYawAxes(yaw, axes);
		obb.orientations[0] = axes[0];
		obb.orientations[1] = axes[1];
		obb.orientations[2] = axes[2];

		// 出現中だけ実サイズ、それ以外はゼロで無効化
		obb.size = (activeTime_ > 0.0f) ? obbHalf_ : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	// === ImGui ===
	/*if (showDebug_) {
		ImGui::Begin("Sword OBB (Front)");
		ImGui::Checkbox("Show Debug", &showDebug_);
		ImGui::DragFloat3("Half Extents", &obbHalf_.x, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Front Dist", &frontDist_, 0.01f, 0.0f, 3.0f);
		ImGui::DragFloat("Height", &height_, 0.01f, 0.0f, 3.0f);
		ImGui::DragFloat("Active Sec", &activeDuration_, 0.01f, 0.05f, 1.0f);
		ImGui::Text("Active: %s (%.2fs)", (activeTime_ > 0.0f ? "ON" : "OFF"), activeTime_);
		ImGui::End();
	}*/
}

void PlayerWeaponOBB::Draw()
{
	//mc_->Draw();

	// 弾描画
	/*for (auto& b : bullets_) {
		b->Draw();
	}*/
}

void PlayerWeaponOBB::NormalAttack()
{
	// クリックの立ち上がりで発動
	if (Input::GetInstance()->TriggerMouseButton(0)) {
		activeTime_ = activeDuration_; // 一定時間だけ有効
		if (owner_) {
			
			owner_->RequestAnimKey(PlayerAnimKey::SwordAttackFast, 10, activeDuration_);
		}
	}
}

void PlayerWeaponOBB::Skill()
{
	const bool nowF = Input::GetInstance()->PushKey(DIK_E);
	if (nowF && !IsSkill_) {                  // 立ち上がりだけ
		if (owner_) {
			owner_->RequestAnimKey(PlayerAnimKey::Roll, 10, 0.8f);
		}
	}
	IsSkill_ = nowF;
}

void PlayerWeaponOBB::Ultimate()
{
	//if (!Input::GetInstance()->TriggerKey(DIK_E)) return;
	//if (!owner_ || !playerTransform_) return;

	//// 発射位置：プレイヤーの頭上あたり
	//Vector3 start = playerTransform_->translate + Vector3{ 0.0f, 1.2f, 0.0f };

	//// 方向：敵がいれば敵方向、いなければプレイヤーの前方
	//Vector3 dir{};
	//bool hasTarget = (enemyTransform_ != nullptr);
	//if (hasTarget) {
	//	dir = enemyTransform_->translate - start;
	//	if (Length(dir) > 0.0001f) dir = Normalize(dir);
	//	else hasTarget = false;
	//}
	//if (!hasTarget) {
	//	const float yaw = playerTransform_->rotate.y;
	//	dir = { std::sin(yaw), 0.0f, std::cos(yaw) };
	//	dir = Normalize(dir);
	//}

	//// 弾を生成
	//auto bullet = std::make_unique<PlayerBullet>(owner_->GetBaseScene());
	//// いま使っているカメラを渡す
	//bullet->SetCamera(owner_->GetCamera());
	//bullet->Initialize();
	//bullet->Fire(start, dir, /*speed*/0.8f, /*lifeSec*/3.0f);

	//bullets_.push_back(std::move(bullet));

	//// アニメ
	//owner_->RequestAnimKey(PlayerAnimKey::SwordAttackFast, 10, 0.6f);
}

#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include "Player.h"

// Yaw(=Y回転)から OBB の3軸を作る簡易ヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	// 右(X), 上(Y), 前(Z)
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f,  0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

// 角度差分を [-π, π] に折りたたむ
static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

// 角度の線形補間（ラップ考慮）
static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	/*ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");*/
	ModelManager::GetInstance()->LoadModel("matest.obj");
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");
	ModelManager::GetInstance()->LoadModel("Sam.gltf");

	object3d_->SetModel("Skeleton.gltf");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f,30.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 3.0f, 3.0f, 3.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->SetAnimation(animation_.Idle);

	// ---- OBB コライダー初期化 ----
	colliderCenter_ = transform.translate + colliderOffset_;

	OBB obb{};
	obb.center = colliderCenter_;
	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];
	obb.size = obbSize_;  // 半径

	Shape first{};
	first.kind = ShapeKind::OBB;
	first.obb = obb;

	*multiCollider_ = MultiCollider(first);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	// ヒットを Enemy::OnCollision に橋渡し
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

	// AI初期
	state_ = RushState::Idle;
	stateTimer_ = idleTime_;

}

void Enemy::Update()
{
	// ====== 簡易AI：Idle → Dash → Cooldown ループ ======
   // ターゲットがいれば計算
	Vector3 toTargetXZ{ 0,0,0 };
	float desiredYaw = transform.rotate.y;

	if (target_) {
		Vector3 to = target_->translate - transform.translate;
		to.y = 0.0f;
		if (Length(to) > 1e-6f) {
			toTargetXZ = Normalize(to);
			// forward = (sinYaw, 0, cosYaw) の想定
			desiredYaw = std::atan2(toTargetXZ.x, toTargetXZ.z);
		}
	}

	switch (state_) {
	case RushState::Idle:
		// 常にプレイヤーへ向く（スムーズ）
		transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_);
		stateTimer_ -= 1.0f / 60.0f;
		if (stateTimer_ <= 0.0f) {
			// ダッシュへ移行：向いている方向を固定
			if (target_) {
				dashDir_ = toTargetXZ;                 // 方向確定（XZ）
				transform.rotate.y = desiredYaw;       // 顔をピタッと正面へ
			}
			else {
				// ターゲットが無い場合は今の向きで前進
				dashDir_ = { std::sin(transform.rotate.y), 0.0f, std::cos(transform.rotate.y) };
			}
			state_ = RushState::Dash;
			stateTimer_ = dashTime_;
			SetAnimationIfChanged(animation_.Run);
		}
		break;

	case RushState::Dash:
		// 突進（※ 突進中はプレイヤーへ向き直ししない）
		transform.translate.x += dashDir_.x * dashSpeed_;
		transform.translate.z += dashDir_.z * dashSpeed_;

		stateTimer_ -= 1.0f / 60.0f;
		if (stateTimer_ <= 0.0f) {
			state_ = RushState::Cooldown;
			stateTimer_ = cooldownTime_;
			SetAnimationIfChanged(animation_.Idle);
		}
		break;

	case RushState::Cooldown:
		// 向きだけは緩やかにターゲットへ（次のダッシュ準備）
		transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_ * 0.6f);
		stateTimer_ -= 1.0f / 60.0f;
		if (stateTimer_ <= 0.0f) {
			state_ = RushState::Idle;
			stateTimer_ = idleTime_;
			SetAnimationIfChanged(animation_.Idle);
		}
		break;
	}
	

	// 当たり判定中心を更新
	//colliderTranslate_ = transform.translate + colliderOffset_;
	colliderCenter_ = transform.translate + colliderOffset_;

	// コライダー更新
	/*Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;*/

	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	// スケールを当たりにも反映したい場合はここで掛ける
	obb.size = { obbSize_.x /** transform.scale.x*/,
				 obbSize_.y /** transform.scale.y*/,
				 obbSize_.z /** transform.scale.z*/ };


	// HitReact の終了管理：一定時間で Idle に戻す
	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= 1.0f / 60.0f; // 固定60FPS前提。可変ならΔtを使う
		if (hitReactTimer_ <= 0.0f) {
			SetAnimationIfChanged(animation_.Idle);
			hitReactTimer_ = 0.0f;
		}
	}

	// ------------------------
	// オブジェクト更新処理
	// ------------------------
	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

}

void Enemy::Draw()
{
	// コライダーの描画
	//multiCollider_->Draw();
}

void Enemy::DrawModel()
{
	object3d_->Draw();
}

void Enemy::SkinningDraw()
{
}

void Enemy::ParticleDraw()
{
}

void Enemy::OnCollision()
{
	// ===== HP減少 =====
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	// ===== HPチェック =====
	if (hp_ <= 0) {
		// 死亡アニメーション再生
		//SetAnimationIfChanged(animation_.Death);
		object3d_->SetAnimationOneShot(animation_.Death);
		hitReactTimer_ = 0.0f; // もうHitReactしない

		// デバッグ出力
		ImGui::Begin("Enemy HP");
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Enemy Died! HP = 0");
		ImGui::End();

		return; // 以降の処理はスキップ
	}

	// ===== 被弾時アニメーション =====
	SetAnimationIfChanged(animation_.HitReact);
	hitReactTimer_ = kHitReactDuration_;

	/*ImGui::Begin("enemy");
	ImGui::Text("On!!!!!!!!!!");
	ImGui::End();*/
}

void Enemy::SetAnimationIfChanged(const std::string& name)
{
	if (currentAnimationName_ != name) {
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}
}

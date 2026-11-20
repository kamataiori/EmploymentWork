#include "PlayerMover.h"
#include "MathFunctions.h"
#include "FollowCamera.h"
#include "IPlayerInput.h"
#include "PostEffectManager.h"
#include "engine/TimeManager.h"

#include <numbers>
#include <cmath>
#include <algorithm>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace {
	// 角度を -π〜π に正規化する
	float WrapPi(float a) {
		while (a > std::numbers::pi_v<float>) a -= 2.0f * std::numbers::pi_v<float>;
		while (a < -std::numbers::pi_v<float>) a += 2.0f * std::numbers::pi_v<float>;
		return a;
	}
}

void PlayerMover::Initialize(Transform* transform)
{
	transform_ = transform;
}

void PlayerMover::SetGroundParams(float sphereRadius, const Vector3& colliderOffset)
{
	sphereRadius_ = sphereRadius;
	colliderOffset_ = colliderOffset;
}

void PlayerMover::Update(bool inputLocked)
{
	// ===== 演出中は入力をすべて無視 =====
	// アニメ（Idle）の決定は Player 側に任せるため、ここでは
	// 「移動入力なし」とだけ伝えて何もしない。
	if (inputLocked) {
		isMoving_ = false;
		// 演出・スキル中は向きの制御をそちらへ譲る。
		// 中断前のソフトロックを持ち越すと、演出が明けてから古い方向へ向いてしまう。
		faceTurn_.active = false;
		return;
	}

	Move();
}

void PlayerMover::Move()
{
	Transform& transform = *transform_;

	// ===== Δt（回転のスムージング用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// 1〜3) 移動入力をカメラ相対の向きへ変換する。
	//    攻撃モーション中(moveLocked_)は入力を拾わない＝足が止まり、向きも変わらない。
	Vector3 wishDir = moveLocked_ ? Vector3{ 0.0f, 0.0f, 0.0f } : ComputeInputDirectionXZ();

	bool isMoving = (Length(wishDir) > 0.0001f);
	if (isMoving) wishDir = Normalize(wishDir);

	// 4) 移動
	const float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;
	if (isMoving) {
		transform.translate.x += wishDir.x * currentSpeed;
		transform.translate.z += wishDir.z * currentSpeed;
	}

	// 5) 目標ヨー角：移動方向そのものを向く（エンドフィールド式）
	//    D で右、A で左、S でカメラ手前（＝画面手前）を向く。
	//    入力が無いフレームは今の向きを維持する（正面へ戻さない）。
	if (isMoving) {
		const float targetYaw = std::atan2(wishDir.x, wishDir.z);

		// 6) スムーズ回転（最短角＆角速度クランプ）
		const float curYaw = transform.rotate.y;
		float deltaYaw = WrapPi(targetYaw - curYaw);
		const float turnRate = std::numbers::pi_v<float> *move_.kTurnSpeed;
		const float maxStep = turnRate * dt;

		if (deltaYaw > maxStep) deltaYaw = maxStep;
		if (deltaYaw < -maxStep) deltaYaw = -maxStep;
		transform.rotate.y = curYaw + deltaYaw;
	}

	isMoving_ = isMoving;

	// 7) ソフトロックの旋回（攻撃開始時に決めた向きへ速く寄せる）
	//    自分で動かしているときは入力の向きを優先する＝ソフトロックは打ち切る。
	if (isMoving) {
		faceTurn_.active = false;
	}
	UpdateFaceTurn(dt);

	// ジャンプ / ブリンク
	Jump();
	Blink();
}

Vector3 PlayerMover::ComputeInputDirectionXZ() const
{
	if (!playerInput_) {
		return { 0.0f, 0.0f, 0.0f };
	}

	// 1) カメラのヨー角（FollowCamera想定）
	float camYaw = 0.0f;
	if (auto fc = dynamic_cast<FollowCamera*>(camera_)) {
		camYaw = fc->GetAngle(); // カメラの“後ろ向き”角
	}

	// 2) カメラ基底ベクトル（XZ）
	const Vector3 cameraForwardXZ = { -std::sin(camYaw), 0.0f, -std::cos(camYaw) };
	const Vector3 cameraRightXZ = { -std::cos(camYaw), 0.0f,  std::sin(camYaw) };

	// 3) 入力は -1〜1 の2軸（キーボード=倒し切り / スティック=傾き）で受け取るだけで、
	//    どのキーか・どのスティックかはシーンの入力クラスが決める。
	//    長さは呼び出し側で使う（移動側は Normalize して速度を一定にする）。
	Vector3 direction = { 0.0f, 0.0f, 0.0f };
	direction += cameraForwardXZ * playerInput_->GetMoveY();  // 前後
	direction += cameraRightXZ * playerInput_->GetMoveX();    // 左右
	return direction;
}

void PlayerMover::StartFaceTurn(float targetYaw, float turnSeconds)
{
	if (!transform_) return;

	// 指定時間で向き切れる角速度を、残り角度から逆算する
	// （大きくズレているほど速く回るので、角度によらず同じ時間で向き終わる）
	const float deltaYaw = std::fabs(WrapPi(targetYaw - transform_->rotate.y));

	if (turnSeconds <= 0.0f) {
		// 時間指定が無ければ即座に向く
		transform_->rotate.y = targetYaw;
		faceTurn_.active = false;
		return;
	}

	faceTurn_.active = true;
	faceTurn_.targetYaw = targetYaw;
	faceTurn_.rate = deltaYaw / turnSeconds;
}

void PlayerMover::UpdateFaceTurn(float dt)
{
	if (!faceTurn_.active || !transform_) return;

	const float curYaw = transform_->rotate.y;
	const float deltaYaw = WrapPi(faceTurn_.targetYaw - curYaw);
	const float maxStep = faceTurn_.rate * dt;

	// 届くなら合わせて終了
	if (std::fabs(deltaYaw) <= maxStep) {
		transform_->rotate.y = faceTurn_.targetYaw;
		faceTurn_.active = false;
		return;
	}

	transform_->rotate.y = curYaw + (deltaYaw > 0.0f ? maxStep : -maxStep);
}

void PlayerMover::Jump()
{
	Transform& transform = *transform_;

	// 有効半径（スケール対応：最大軸で拡大）
	const float effectiveRadius =
		sphereRadius_ * std::max({ transform.scale.x, transform.scale.y, transform.scale.z });

	// 現在の当たり中心Y（足元原点 + オフセット）
	const float colliderCenterY = transform.translate.y + colliderOffset_.y;

	// 現在の足底Y
	const float bottomNow = colliderCenterY - effectiveRadius;

	// 接地判定（ほぼ地面にいるか）
	const bool isGrounded = (bottomNow <= jump_.kGroundHeight + 0.0001f);

	// 1 接地クランプ（非ジャンプ時の保険）
	if (!jump_.isJumping && bottomNow < jump_.kGroundHeight) {
		const float targetCenterY = jump_.kGroundHeight + effectiveRadius;
		transform.translate.y = targetCenterY - colliderOffset_.y;

		// 着地リセット
		jump_.isJumping = false;
		jump_.velocity = 0.0f;
		jump_.jumpCount = 0;
		jump_.canJump_ = true;
		move_.hasDashed_ = false;
	}

	// 2 ジャンプ入力（地面にいる時だけ / 1段のみ）
	const bool jumpHeld = playerInput_ && playerInput_->IsJumpHeld();
	if (isGrounded && jump_.canJump_ && jumpHeld) {
		jump_.velocity = jump_.kInitialVelocity;
		jump_.isJumping = true;
		jump_.jumpCount = 1;      // 保持したいなら（なくてもOK）
		jump_.canJump_ = false;   // 離すまで再ジャンプ禁止
	}
	if (!jumpHeld) {
		jump_.canJump_ = true;
	}

	// 3 速度反映（予測→着地判定→クランプ）
	if (jump_.isJumping) {
		const float nextY = transform.translate.y + jump_.velocity;

		const float nextCenterY = nextY + colliderOffset_.y;
		const float bottomNext = nextCenterY - effectiveRadius;

		if (jump_.velocity <= 0.0f && bottomNext < jump_.kGroundHeight) {
			const float targetCenterY = jump_.kGroundHeight + effectiveRadius;
			transform.translate.y = targetCenterY - colliderOffset_.y;

			// 着地リセット
			jump_.isJumping = false;
			jump_.velocity = 0.0f;
			jump_.jumpCount = 0;
			jump_.canJump_ = true;
			move_.hasDashed_ = false;
		}
		else {
			transform.translate.y = nextY;
			jump_.velocity -= jump_.kGravity;
		}
	}
}

void PlayerMover::Blink()
{
	Transform& transform = *transform_;

	// ===== Δt（クールダウン・ダッシュ時間用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// -------------------------------
	// ダッシュ制御：1回だけ発動可能
	// -------------------------------
	// クールダウンを減算
	if (move_.dashCooldown > 0.0f) {
		move_.dashCooldown -= dt;
		if (move_.dashCooldown < 0.0f) move_.dashCooldown = 0.0f;
	}

	// 押下・解放状態（ダッシュ操作）
	const bool dashHeld = playerInput_ && playerInput_->IsDashHeld();

	// 接地判定
	const float groundEps = 0.001f;
	const bool isGrounded = (!jump_.isJumping) && (transform.translate.y <= jump_.kGroundHeight + groundEps);

	// ---- 起動条件：押した・ダッシュ中でない・クールダウン終わり ----
	if (!move_.isDashing && dashHeld && !move_.isDashKeyHeld_ && move_.dashCooldown <= 0.0f) {
		move_.isDashing = true;
		move_.dashTimer = move_.kDashDuration;
		move_.hasDashed_ = true;
		move_.isDashKeyHeld_ = true;

		const float yaw = transform.rotate.y;
		move_.dashDir = Normalize(Vector3{ std::sin(yaw), 0.0f, std::cos(yaw) });

		PostEffectManager::GetInstance()->SetType(PostEffectType::RadialBlur);
	}

	// 解放を検出（次の押下のためのエッジ作り）
	if (!dashHeld) {
		move_.isDashKeyHeld_ = false;
	}

	// ---- ダッシュ中の処理 ----
	if (move_.isDashing) {
		move_.dashTimer -= dt;
		transform.translate.x += move_.dashDir.x * move_.dashSpeed;
		transform.translate.z += move_.dashDir.z * move_.dashSpeed;

		if (move_.dashTimer <= 0.0f) {
			move_.isDashing = false;
			move_.dashTimer = 0.0f;
			move_.dashCooldown = move_.kDashCooldown;      // 終了後にクールダウン開始
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
		}
	}
	else {
		// ---- 再装填条件：クールダウン終了 ＆ ダッシュ操作を離している ＆（できれば接地）----
		if (move_.dashCooldown <= 0.0f && !dashHeld && isGrounded) {
			move_.hasDashed_ = false;  // これで2回目以降も使える
		}
	}
}

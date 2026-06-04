#include "PlayerMover.h"
#include "MathFunctions.h"
#include "FollowCamera.h"
#include "Input.h"
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
		return;
	}

	Move();
}

void PlayerMover::Move()
{
	Transform& transform = *transform_;

	// ===== Δt（回転のスムージング用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// 1) カメラのヨー角（FollowCamera想定）
	float camYaw = 0.0f;
	if (auto fc = dynamic_cast<FollowCamera*>(camera_)) {
		camYaw = fc->GetAngle(); // カメラの“後ろ向き”角
	}

	// カメラ正面（プレイヤーが向き続けるべき前）
	const float playerFaceYaw = camYaw + std::numbers::pi_v<float>;

	// 2) カメラ基底ベクトル（XZ）
	const Vector3 cameraForwardXZ = { -std::sin(camYaw), 0.0f, -std::cos(camYaw) };
	const Vector3 cameraRightXZ = { -std::cos(camYaw), 0.0f,  std::sin(camYaw) };

	// 3) WASDをカメラ相対で合成（D=+Right / A=-Right）
	Vector3 wishDir = { 0,0,0 };
	if (Input::GetInstance()->PushKey(DIK_W)) wishDir += cameraForwardXZ;
	if (Input::GetInstance()->PushKey(DIK_S)) wishDir -= cameraForwardXZ;
	if (Input::GetInstance()->PushKey(DIK_D)) wishDir += cameraRightXZ;
	if (Input::GetInstance()->PushKey(DIK_A)) wishDir -= cameraRightXZ;

	bool isMoving = (Length(wishDir) > 0.0001f);
	if (isMoving) wishDir = Normalize(wishDir);

	// 4) 移動
	const float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;
	if (isMoving) {
		transform.translate.x += wishDir.x * currentSpeed;
		transform.translate.z += wishDir.z * currentSpeed;
	}

	// 5) 目標ヨー角：常に「正面固定」（移動方向では回さない！）
	const float targetYaw = playerFaceYaw;

	// 6) スムーズ回転（最短角＆角速度クランプ）
	const float curYaw = transform.rotate.y;
	float deltaYaw = WrapPi(targetYaw - curYaw);
	const float turnSpeed = 2.5f;
	const float turnRate = std::numbers::pi_v<float> *turnSpeed; // 180deg/s
	const float maxStep = turnRate * dt;

	if (deltaYaw > maxStep) deltaYaw = maxStep;
	if (deltaYaw < -maxStep) deltaYaw = -maxStep;
	transform.rotate.y = curYaw + deltaYaw;

	isMoving_ = isMoving;

	// ジャンプ / ブリンク
	Jump();
	Blink();
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
	if (isGrounded && jump_.canJump_ && Input::GetInstance()->PushKey(DIK_SPACE)) {
		jump_.velocity = jump_.kInitialVelocity;
		jump_.isJumping = true;
		jump_.jumpCount = 1;      // 保持したいなら（なくてもOK）
		jump_.canJump_ = false;   // 離すまで再ジャンプ禁止
	}
	if (!Input::GetInstance()->PushKey(DIK_SPACE)) {
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

	// 押下・解放状態
	const bool dashHeld = Input::GetInstance()->PushMouseButton(1);

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

	// 右クリック解放を検出（次の押下のためのエッジ作り）
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
		// ---- 再装填条件：クールダウン終了 ＆ 右クリック離している ＆（できれば接地）----
		if (move_.dashCooldown <= 0.0f && !dashHeld && isGrounded) {
			move_.hasDashed_ = false;  // これで2回目以降も使える
		}
	}
}

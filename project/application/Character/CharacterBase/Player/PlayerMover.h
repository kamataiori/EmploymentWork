#pragma once
#include "Transform.h"
#include "Vector3.h"

class Camera;

//======================================================
// PlayerMover
//------------------------------------------------------
// プレイヤーの移動・ジャンプ・ブリンク(ダッシュ)を担当するコンポーネント
// Transform は所有せず、Player の Transform を参照(ポインタ)で借りて動かす
//======================================================
class PlayerMover {
public:

	/// <summary>
	/// 初期化。Player から動かす対象の Transform を借りる。
	/// </summary>
	/// <param name="transform">Player が所有する Transform への参照</param>
	void Initialize(Transform* transform);

	/// <summary>
	/// カメラをセット（向き計算に FollowCamera のヨー角を使う）
	/// </summary>
	void SetCamera(Camera* camera) { camera_ = camera; }

	/// <summary>
	/// 接地計算に使うコライダー情報を Player と共有する。
	/// </summary>
	/// <param name="sphereRadius">球コライダー半径</param>
	/// <param name="colliderOffset">足元原点からの中心オフセット</param>
	void SetGroundParams(float sphereRadius, const Vector3& colliderOffset);

	/// <summary>
	/// 毎フレーム更新。inputLocked=true（演出中など）の間は移動入力を無視する。
	/// </summary>
	void Update(bool inputLocked);

	/// <summary>
	/// 移動入力があったか（アニメ Run/Idle 判定用）
	/// </summary>
	bool IsMoving() const { return isMoving_; }

	/// <summary>
	/// ダッシュ中か
	/// </summary>
	bool IsDashing() const { return move_.isDashing; }

private:

	/// <summary>
	/// 全player共通の動き
	/// </summary>
	void Move();

	/// <summary>
	/// playerのジャンプ処理
	/// </summary>
	void Jump();

	/// <summary>
	/// playerのブリンク(ダッシュ)処理
	/// </summary>
	void Blink();

private:

	// 動かす対象（Player が所有。ここでは借りるだけで delete しない）
	Transform* transform_ = nullptr;

	// 向き計算用カメラ（共有）
	Camera* camera_ = nullptr;

	// 接地計算用（Player の本体コライダーと同じ値）
	float sphereRadius_ = 1.0f;
	Vector3 colliderOffset_ = {};

	// 移動入力があったか（アニメ判定用の出力）
	bool isMoving_ = false;

	// 移動制御に関する構造体
	struct MoveControl {
		float speed = 0.25f;                    // 移動速度
		Vector3 direction{};                   // 入力から得た移動方向
		float dashSpeed = 1.0f;                // ダッシュ時の速度
		bool isDashing = false;                // ダッシュ中かどうか
		bool hasDashed_ = false;               // 1回だけダッシュ許可
		bool isDashKeyHeld_ = false;           // キーがまだ押されているかどうか
		const float kDashDuration = 0.2f;      // ダッシュ継続時間
		float dashTimer = 0.0f;                // 残りダッシュ時間
		Vector3 dashDir = { 0.0f, 0.0f, 0.0f };// ダッシュ開始時の向き
		const float kDashCooldown = 0.25f; // 連打防止の再装填時間
		float dashCooldown = 0.0f;
	};
	MoveControl move_;  // 移動制御

	// ジャンプに関するデータ構造体
	struct JumpControl {
		bool isJumping = false;               // ジャンプ中か
		float velocity = 0.0f;                // 上下速度
		int jumpCount = 0;                    // 現在ジャンプ回数
		const int kMaxJumpCount = 1;          // 最大ジャンプ回数
		bool canJump_ = true;                 // Spaceキーが離されたことを確認するフラグ
		const float kInitialVelocity = 0.19f;  // 初速（頂点 ≈ v^2/2g ≈ 1.44ユニット）
		const float kGravity = 0.0125f;        // 重力（滞空 ≈ 2v/g ≈ 0.5秒）
		const float kGroundHeight = 0.0f;     // 地面高さ
	};
	JumpControl jump_;
};

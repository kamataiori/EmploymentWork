#pragma once
#include "Camera/Camera.h"
#include "ObjectBase.h"

class FollowCamera : public Camera
{
public:
	FollowCamera(ObjectBase* target, float followDistance, float heightOffset);

	/// <summary>
	/// 更新（追従処理）
	/// </summary>
	void Update() override;

public: // カメラとターゲットとの距離関係

	/// <summary>
	/// カメラとターゲットとの距離を設定
	/// </summary>
	void SetFollowDistance(float distance) { followDistance = distance; }

	/// <summary>
	/// カメラとターゲットとの距離を取得
	/// </summary>
	float GetFollowDistance() const { return followDistance; }

	/// <summary>
	/// ターゲットを後から設定する
	/// </summary>
	void SetTarget(ObjectBase* newTarget) {
		target = newTarget; 
		initializedAngle_ = false;   // ← ターゲット変更時にリセット
		initializedPosY_ = false;
	}

	/// <summary>
	/// 現在のターゲットを取得する
	/// </summary>
	ObjectBase* GetTarget() const { return target; }

public: // カメラの高さ関係

	/// <summary>
	/// 高さのオフセットを設定
	/// </summary>
	void SetHeightOffset(float offset) { heightOffset = offset; }

	/// <summary>
	/// 高さのオフセットを取得
	/// </summary>
	float GetHeightOffset() const { return heightOffset; }

	/// <summary>
	/// 注視点の高さ（画面中央に映る点。頭の高さ）を設定
	/// </summary>
	void SetLookHeight(float h) { lookHeight_ = h; }

	/// <summary>
	/// 注視点の高さを取得
	/// </summary>
	float GetLookHeight() const { return lookHeight_; }

	/// <summary>
	/// カメラの回転角度（Y軸）を設定
	/// </summary>
	void SetAngle(float a) { angle = a; }

	/// <summary>
	/// カメラの回転角度（Y軸）を取得
	/// </summary>
	float GetAngle() const { return angle; }

	void SetSensitivity(float s) { sensitivity_ = s; }
	float GetSensitivity() const { return sensitivity_; }

	void SetLockLookY(bool v) { lockLookY_ = v; }
	void SetLookYSmooth(float v) { lookYSmooth_ = v; }


private: // メンバ変数

	ObjectBase* target; // キャラクター（PlayerやEnemyなど）
	float followDistance;  // 対象との距離
	float heightOffset = 1.5f;    // カメラの高さオフセット
	float angle = 3.14f;    // カメラの回転角度（Y軸）
	float sensitivity_ = 0.001f; // 初期値：感度

	// --- 注視点（画面中央に映る点）---
	float lookHeight_ = 2.4f;        // 注視点の高さ（プレイヤー基準・頭の高さ）

	// --- 肩越し（カメラ／注視点の横ずらし量）---
	float cameraSideShift_ = 1.3f;   // 横へずらす量（大きいほどキャラが画面端寄り）

	// --- カメラ操作 ---
	float keyOrbitSpeed_ = 0.01f;    // キーボードでの周回速度（rad/frame）

	// --- 上下の見下ろし角(pitch)：マウス上下で操作 ---
	float cameraPitch_ = 0.0f;         // 現在の見下ろし角（マウスYで変化。rad）
	float pitchSensitivity_ = 0.0025f; // マウス上下の感度
	float pitchMin_ = -0.25f;          // 上限（負＝見上げ側）
	float pitchMax_ = 0.5f;            // 下限（下を向きすぎないため）

	bool initializedAngle_ = false;
	bool initializedPosY_ = false;
	bool initializedPitch_ = false;

	// --- 注視点Yの追従を弱める設定 ---
	bool  lockLookY_ = true;          // true: 注視点Yを固定（追従しない）
	float lookY_ = 0.0f;              // 注視点Y（固定/スムーズ用の保持値）
	float lookYSmooth_ = 8.0f;        // lockLookY_=false時の追従の弱さ（大きいほど追従強）
	bool  dampPosY_ = true;      // true: カメラ位置Yを弱追従
	float posYSmooth_ = 6.0f;    // 大きいほど追従が速い
};

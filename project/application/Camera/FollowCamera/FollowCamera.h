#pragma once
#include "Camera.h"
#include "CharacterBase.h"

class FollowCamera : public Camera
{
public:
	FollowCamera(CharacterBase* target, float followDistance, float heightOffset);

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
	/// カメラの回転角度（Y軸）を設定
	/// </summary>
	void SetAngle(float a) { angle = a; }

	/// <summary>
	/// カメラの回転角度（Y軸）を取得
	/// </summary>
	float GetAngle() const { return angle; }

	void SetSensitivity(float s) { sensitivity_ = s; }
	float GetSensitivity() const { return sensitivity_; }

private: // メンバ変数

	CharacterBase* target; // キャラクター（PlayerやEnemyなど）
	float followDistance;  // 対象との距離
	float heightOffset;    // カメラの高さオフセット
	float angle = 3.14f;    // カメラの回転角度（Y軸）
	float sensitivity_ = 0.001f; // 初期値：感度
};

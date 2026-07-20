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

	/// <summary>
	/// マウスによるカメラ操作の有効/無効を切り替える。
	/// （スキル発動中など、カメラを固定したい場面で false にする）
	/// </summary>
	void SetMouseControlEnabled(bool v) { mouseControlEnabled_ = v; }
	bool IsMouseControlEnabled() const { return mouseControlEnabled_; }

	/// <summary>
	/// マウス感度の一時的な倍率（yaw/pitch 両方に掛かる）。
	/// アルティメットのスローモーション中など、カメラをゆっくり動かしたい場面で下げる。
	/// 1.0=通常。演出が終わったら 1.0 に戻すこと。
	/// </summary>
	void SetMouseSensitivityScale(float s) { mouseSensitivityScale_ = s; }
	float GetMouseSensitivityScale() const { return mouseSensitivityScale_; }

	/// <summary>
	/// シネマティック上書き（アルティメット等の演出用）。
	/// 毎フレーム、演出側から「望むカメラ位置 / 注視点 / ブレンド率(0=通常追従,1=指定ポーズ)」を与える。
	/// weight を 0→1→0 と補間すれば、追従⇔演出の出入りが滑らかになる。
	/// </summary>
	void SetCinematicBlend(const Vector3& pos, const Vector3& lookAt, float weight) {
		cinePos_ = pos;
		cineLook_ = lookAt;
		cineWeight_ = weight;
	}
	/// <summary>シネマティック上書きを解除して通常追従へ戻す。</summary>
	void ClearCinematic() { cineWeight_ = 0.0f; }
	float GetCinematicWeight() const { return cineWeight_; }

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

	// --- パッド右スティックでのカメラ操作 ---
	// スティックは -1.0〜1.0 で返るので、倒し切ったときの1フレームの回転量を指す。
	// マウスは移動量(px)にかかる係数なので、そちらとは別の値が要る。
	float padOrbitSpeed_ = 0.035f;   // 左右：周回速度（rad/frame）
	float padPitchSpeed_ = 0.02f;    // 上下：見下ろし角の速度（rad/frame）

	// --- 上下の見下ろし角(pitch)：マウス上下で操作 ---
	float cameraPitch_ = 0.0f;         // 現在の見下ろし角（マウスYで変化。rad）
	float pitchSensitivity_ = 0.0025f; // マウス上下の感度
	float pitchMin_ = -0.25f;          // 上限（負＝見上げ側）
	float pitchMax_ = 0.5f;            // 下限（下を向きすぎないため）

	// --- マウス操作のロック ---
	bool mouseControlEnabled_ = true;  // false の間はマウスでカメラを動かせない
	float mouseSensitivityScale_ = 1.0f; // マウス感度の一時倍率（スロー演出中に下げる）

	// --- シネマティック上書き（演出側が pos/look/weight を与える）---
	Vector3 cinePos_{};       // 望むカメラ位置
	Vector3 cineLook_{};      // 望む注視点
	float   cineWeight_ = 0.0f; // 0=通常追従 / 1=指定ポーズ

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

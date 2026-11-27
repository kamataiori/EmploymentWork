#pragma once
#include "Camera/Camera.h"
#include "ObjectBase.h"
#include "ICameraInput.h"
#include "ICameraBlocker.h"

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

	/// <summary>
	/// 視点入力の読み取り口を注入する（実装はシーンの GameSceneInput。所有はしない）。
	/// 未注入の間はカメラ操作を受け付けない（追従だけ行う）。
	/// どのデバイスをどの感度で使うかは入力側が持つ。
	/// </summary>
	void SetCameraInput(const ICameraInput* input) { cameraInput_ = input; }

	/// <summary>
	/// カメラを遮る形状（ステージ）の供給元を注入する（所有はしない）。
	/// 未注入ならめり込み防止は働かず、従来どおり常に腕を伸ばし切る。
	/// </summary>
	void SetCameraBlocker(const ICameraBlocker* blocker) { cameraBlocker_ = blocker; }

	/// <summary>
	/// めり込み防止の調整用 ImGui（Debug ビルドのみ中身がある）。シーンの Debug() から呼ぶ。
	/// </summary>
	void DebugImGui();

	/// <summary>
	/// マウスによるカメラ操作の有効/無効を切り替える。
	/// （スキル発動中など、カメラを固定したい場面で false にする）
	/// </summary>
	void SetMouseControlEnabled(bool v) { mouseControlEnabled_ = v; }
	bool IsMouseControlEnabled() const { return mouseControlEnabled_; }

	/// <summary>
	/// マウス/スティック感度の一時的な倍率（yaw/pitch 両方に掛かる）。
	/// アルティメットのスローモーション中など、カメラをゆっくり動かしたい場面で下げる。
	/// 1.0=通常。演出が終わったら 1.0 に戻すこと。
	/// (キーボードでの旋回は一定速度のままで、この倍率の対象外)
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


private: // メンバ関数

	/// <summary>
	/// めり込み防止：注視点からカメラへの「腕」を壁に当たる手前まで縮める。
	/// 向きは変えず、腕の長さだけを差し替える。
	/// </summary>
	/// <param name="pivot">腕の根元（注視点）</param>
	/// <param name="cinematic">見下ろし演出中か（その間は判定しない）</param>
	void UpdateCollisionPullIn(const Vector3& pivot, bool cinematic, float dt);

private: // メンバ変数

	ObjectBase* target; // キャラクター（PlayerやEnemyなど）
	float followDistance;  // 対象との距離
	float heightOffset = 1.5f;    // カメラの高さオフセット
	float angle = 3.14f;    // カメラの回転角度（Y軸）

	// 視点入力の読み取り口（シーンが所有。ここでは借りるだけ）
	const ICameraInput* cameraInput_ = nullptr;

	// --- めり込み防止（スプリングアーム）---
	// 注視点からカメラへ伸ばす「腕」を、壁に当たる手前まで縮める。
	// 遮る形状の供給元は Scene が注入する（ここでは借りるだけ）。
	const ICameraBlocker* cameraBlocker_ = nullptr;
	bool  collisionEnabled_ = true;       // 切れば従来どおり
	float collisionRadius_ = 0.4f;        // 掃く球の半径[m]。ニアクリップより大きくする
	float collisionEscape_ = 0.05f;       // 当たった位置から壁側へ残す余白[m]
	float collisionMinDistance_ = 0.8f;   // これ以上は詰めない（頭に寄り過ぎないため）[m]
	float collisionReturnSharp_ = 6.0f;   // 腕を戻す速さ[1/秒]。詰めるのは即時、戻すのはゆっくり

	float collisionDistance_ = 0.0f;      // 今フレーム実際に使っている腕の長さ[m]
	bool  initializedCollisionDistance_ = false;

	// --- 注視点（画面中央に映る点）---
	float lookHeight_ = 2.4f;        // 注視点の高さ（プレイヤー基準・頭の高さ）

	// --- 肩越し（カメラ／注視点の横ずらし量）---
	float cameraSideShift_ = 1.3f;   // 横へずらす量（大きいほどキャラが画面端寄り）

	// --- 上下の見下ろし角(pitch) ---
	// 感度や割り当てデバイスは入力側（GameSceneInput の割り当て表）が持つ。
	// ここに残すのは「カメラ側の都合」である可動域だけ。
	float cameraPitch_ = 0.0f;         // 現在の見下ろし角（rad）
	float pitchMin_ = -0.25f;          // 上限（負＝見上げ側）
	float pitchMax_ = 0.5f;            // 下限（下を向きすぎないため）

	// --- 視点操作のロック ---
	bool mouseControlEnabled_ = true;  // false の間はマウス/スティックでカメラを動かせない
	float mouseSensitivityScale_ = 1.0f; // 感度の一時倍率（スロー演出中に下げる）

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

#pragma once
#include "ObjectBase.h"
#include <string>

class Player;

class Sword : public ObjectBase
{
public:
	Sword(BaseScene* scene) : ObjectBase(scene) {}

	void Initialize() override;
	void Update() override;

	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;
	void ParticleDraw() override;
	void AnimationDraw() override;
	void OnCollision() override;

	void AttachTo(Object3d* ownerObj, const std::string& jointName);

	void SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s);

	void SetHitEnabled(bool enabled) { hitEnabled_ = enabled; }
	// 攻撃中で当たり判定が有効か（Scene のコライダー登録判定に使う）
	bool IsHitEnabled() const { return hitEnabled_; }

	MultiCollider* GetMultiCollider() const { return multiCollider_.get(); }

	// プレイヤーのTransformを参照して向き(yaw)を取る
	void SetPlayerTransform(const Transform* t) { playerTransform_ = t; }

	//======================================================
	// スキル「回転斬り（ブーメラン弧）」
	//   E キーで発動。剣を手から切り離し、プレイヤーを中心に
	//   左→右へ大きな弧を描いて自転しながら手元へ戻ってくる。
	//   弧モード中は当たり判定を常時ONにする。
	//======================================================

	/// <summary>
	/// スキル開始：手から切り離してワールド空間の弧モードへ入る。
	/// </summary>
	void BeginSpinArc();

	/// <summary>
	/// スキルの進行度を渡す（0=手元から発射 / 1=手元へ帰還）。
	/// </summary>
	void SetSpinArcProgress(float progress) { spinArcProgress_ = progress; }

	/// <summary>
	/// スキル終了：手に戻して通常のボーン追従へ復帰する。
	/// </summary>
	void EndSpinArc();

	/// <summary>
	/// スキルの弧モード中か（コライダー登録の判定にも使う）。
	/// </summary>
	bool IsSpinArcActive() const { return spinArcActive_; }

private:

	/// <summary>
	/// 進行度から弧上のワールドTransform（位置・自転）を計算して反映する。
	/// </summary>
	void UpdateSpinArcTransform();

private:
	Object3d* ownerObj_ = nullptr;
	std::string ownerJoint_;

	const Transform* playerTransform_ = nullptr;

	bool hitEnabled_ = false;
	bool isHit_ = false;

	// ===== スキル「回転斬り」状態 =====
	bool  spinArcActive_ = false;   // 弧モード中か
	float spinArcProgress_ = 0.0f;  // 弧の進行度 0〜1
	float spinArcYaw_ = 0.0f;       // 発動時のプレイヤーの向き[ラジアン]（弧の方向を固定する）

	// --- 弧の形を決める調整パラメータ（プレイヤー基準） ---
	// Attack02 はプレイヤーから見て「左→右」に振るので、弧も左から右へ薙ぐ。
	// 角度はプレイヤーの正面(向いている方向)からの相対ヨー角[ラジアン]。
	// 左右が反転して見える場合は Start/End の符号を入れ替える。
	const float kArcStartAngle_ = -2.0f;  // 弧の開始角（プレイヤーの右側）
	const float kArcEndAngle_   = +2.0f;  // 弧の終了角（プレイヤーの左側）
	const float kArcMaxRadius_  = 10.0f;   // プレイヤーから最も離れる距離
	const float kArcBaseHeight_ = 1.0f;   // 足元からの基準の高さ（胸あたり）
	const float kArcLiftHeight_ = 1.5f;   // 弧の頂点で持ち上がる高さ
	const float kArcSpinTurns_  = 4.0f;   // 弧の間に剣が自転する回数（ぐるぐる）

	// スキル中の当たり判定OBBの半サイズ（通常攻撃より大きくして当てやすくする）
	const Vector3 kSpinArcHitHalfSize_ = { 2.0f, 1.5f, 3.5f };

	// OBB調整値
	Vector3 hitOffset_ = { 0.0f, 0.0f, 8.5f };
	Vector3 hitHalfSize_ = { 0.7f, 0.8f, 2.2f };

	Vector3 defaultOffsetT_ = { -0.44f, -0.73f, -0.48f };
	Vector3 defaultOffsetR_ = { -0.56f, 4.95f, 0.0f };
	Vector3 defaultOffsetS_ = { 1.0f, 1.0f, 1.0f };
};
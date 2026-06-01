#pragma once
#include "IPlayerSkill.h"
#include "Vector3.h"

class Player;
class Sword;

//======================================================
// SpinSlashSkill（回転斬り / ブーメラン弧）
//------------------------------------------------------
// E キーで発動するプレイヤースキル。
//   発動した瞬間のプレイヤーの向きへ、剣を手から切り離して
//   左→右の大きな弧を描かせ、横向きのまま自転しながら手元へ戻す。
//   発動中は当たり判定を大きめにして常時ONにする。
//
// 役割分担：
//   - 弧の「振り付け」（軌道・自転・時間）はこのクラスが持つ。
//   - 剣の「能力」（切り離し/復帰・ワールド配置・当たり判定）は Sword が持つ。
//   - 借り物（owner_ / sword_）は所有しない（Player が所有）。
//======================================================
class SpinSlashSkill : public IPlayerSkill {
public:

	/// <summary>
	/// 依存を注入する。owner はアニメ再生、sword は弧の駆動に使う。
	/// </summary>
	void Initialize(Player* owner, Sword* sword);

	// ---- IPlayerSkill ----
	void Start() override;
	void Update(float dt) override;
	bool IsActive() const override { return active_; }

private:

	/// <summary>
	/// 進行度 t(0〜1) から、剣のワールド位置・姿勢を計算して Sword へ反映する。
	/// </summary>
	void ApplyArcTransform(float t);

private:

	// 借り物（所有しない）
	Player* owner_ = nullptr;
	Sword*  sword_ = nullptr;

	// ---- 状態 ----
	bool  active_ = false;     // 発動中か
	float elapsed_ = 0.0f;     // 発動からの経過時間[秒]
	float fixedYaw_ = 0.0f;    // 発動時に固定したプレイヤーの向き[ラジアン]

	// ---- スキル全体の設定 ----
	const float kDuration_ = 1.1f;       // スキルの長さ[秒]（弧＋当たり判定の継続時間）
	const float kAnimSpeed_ = 1.0f;      // 再生する Attack02 の速度倍率
	const int   kAnimaPriority_ = 20;    // アニメ優先度（ノーマル攻撃=10 より高く）

	// ---- 弧の形を決める調整パラメータ（プレイヤー基準） ----
	// Attack02 はプレイヤーから見て「左→右」に振るので、弧も左から右へ薙ぐ。
	// 角度は発動時の向きからの相対ヨー角[ラジアン]。左右が逆なら符号を入れ替える。
	const float kArcStartAngle_ = -2.0f;  // 弧の開始角（右側）
	const float kArcEndAngle_   = +2.0f;  // 弧の終了角（左側）
	const float kArcMaxRadius_  = 10.0f;  // プレイヤーから最も離れる距離
	const float kArcBaseHeight_ = 1.0f;   // 足元からの基準の高さ（胸あたり）
	const float kArcLiftHeight_ = 1.5f;   // 弧の頂点で持ち上がる高さ
	const float kArcSpinTurns_  = 4.0f;   // 弧の間に剣が自転する回数（ぐるぐる）

	// 発動中の当たり判定OBBの半サイズ（通常攻撃より大きくして当てやすくする）
	const Vector3 kHitHalfSize_ = { 2.0f, 1.5f, 3.5f };
};

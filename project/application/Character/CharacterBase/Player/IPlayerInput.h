#pragma once

//======================================================
// IPlayerInput
//------------------------------------------------------
// プレイヤー（Player / PlayerMover / PlayerWeapon）が必要とする入力の読み取り口。
//
// なぜインターフェースを挟むか:
//   実装はシーン側の GameSceneInput だが、Player 層が Scene 層のクラスを
//   直接 include すると Scene → Character → Scene の循環になる。
//   「プレイヤーが欲しい入力はこれだけ」という口をこちら側で決めておき、
//   シーンがそれを満たす形にする（依存の向きを一方向に保つ）。
//   既存の PlayerIWeapon / IEnemyTargetProvider / IPlayerSkill と同じやり方。
//
// 値は「今フレームのスナップショット」。実装側が毎フレーム先頭で1回更新する。
//======================================================
class IPlayerInput
{
public:
	virtual ~IPlayerInput() = default;

	///========================
	// 移動（カメラ相対で使う。-1.0〜1.0）
	///========================

	/// 左右 (-1:左 〜 +1:右)
	virtual float GetMoveX() const = 0;
	/// 前後 (-1:手前 〜 +1:奥)
	virtual float GetMoveY() const = 0;

	///========================
	// 押しっぱなし
	///========================

	/// ジャンプ（押している間だけ上昇判定に使う）
	virtual bool IsJumpHeld() const = 0;
	/// ダッシュ（ブリンク）
	virtual bool IsDashHeld() const = 0;
	/// 攻撃（コンボ継続の予約に使う）
	virtual bool IsAttackHeld() const = 0;

	///========================
	// 押した瞬間
	///========================

	/// 通常攻撃の開始
	virtual bool IsAttackTriggered() const = 0;
	/// スキル1（回転斬り）
	virtual bool IsSkillTriggered() const = 0;
	/// スキル2（突進乱舞）
	virtual bool IsSkill2Triggered() const = 0;
	/// アルティメット（斬奪）
	virtual bool IsUltimateTriggered() const = 0;
};

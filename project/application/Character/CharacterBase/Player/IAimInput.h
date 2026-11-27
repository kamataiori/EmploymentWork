#pragma once

//======================================================
// IAimInput
//------------------------------------------------------
// アルティメット（斬奪）の照準＝画面上の斬撃線が必要とする入力の読み取り口。
//
// IPlayerInput と分けている理由:
//   移動や攻撃を読む PlayerMover / PlayerWeapon は照準を知る必要が無く、
//   照準を使うのはこのスキルだけ。読む側ごとに必要な分だけの口にしてある。
//
// 線の位置そのものは持たない（スキル側の状態）。
// ここが担うのは「今フレームの入力でどこへ動くか」の計算だけ。
//======================================================
class IAimInput
{
public:
	virtual ~IAimInput() = default;

	/// <summary>
	/// 縦線（画面X）を今フレームの入力で動かした位置を返す[px]。
	/// </summary>
	/// <param name="currentX">今の位置[px]</param>
	/// <param name="deltaTime">経過時間[秒]。スロー演出中もそのままの速さで動かすなら unscaled</param>
	virtual float MoveAimX(float currentX, float deltaTime) const = 0;

	/// <summary>横線（画面Y）を今フレームの入力で動かした位置を返す[px]。</summary>
	virtual float MoveAimY(float currentY, float deltaTime) const = 0;

	/// <summary>
	/// 線の初期位置[px]（可動範囲の中央＝画面中央）。
	/// 演出開始時に線を中央へ置くのに使う。画面サイズは入力側の割り当て表が持つ。
	/// </summary>
	virtual float GetAimCenterX() const = 0;
	virtual float GetAimCenterY() const = 0;

	/// 斬撃を確定した瞬間
	virtual bool IsAimConfirmTriggered() const = 0;

	/// 見下ろしカメラの左右を入れ替えた瞬間
	virtual bool IsAimSideSwapTriggered() const = 0;
};

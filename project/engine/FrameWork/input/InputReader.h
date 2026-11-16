#pragma once
#include "Input.h"

///========================
// 割り当てに使う種類
///========================

/// <summary>マウスボタン（Input の生インデックスに名前を付けたもの）</summary>
enum class MouseButton : int {
	None = -1,   // 未割り当て
	Left = 0,
	Right = 1,
	Middle = 2,
};

/// <summary>アナログトリガー（LT/RT）をボタンとして割り当てるとき用</summary>
enum class PadTrigger {
	None,        // 未割り当て
	Left,
	Right,
};

/// <summary>マウスの1軸（視点操作のように移動量を使うとき用）</summary>
enum class MouseAxis {
	None,        // 未割り当て
	X,
	Y,
};

/// <summary>スティックの1軸（軸の割り当てに使う）</summary>
enum class StickAxis {
	None,        // 未割り当て（キーボードだけの軸）
	LeftX,
	LeftY,
	RightX,
	RightY,
};

/// <summary>
/// キー未割り当てを表すキーコード。
/// DirectInput のキーコードは DIK_ESCAPE(0x01) から始まるので 0x00 は空き。
/// </summary>
inline constexpr BYTE kNoKey = 0x00;

/// <summary>
/// 生の入力デバイス (Input) を「今押されているか」だけに絞って読む窓口。
///
/// 役割:
///   - キーボード / マウス / パッドの API 差 (PushKey / PushMouseButton /
///     PushButton / PushRightTrigger …) を 1 つの形にそろえる
///   - 未接続・未割り当て・Input 未注入を、すべて「押されていない / 0」に正規化する
///
/// 設計:
///   - 継承しない。入力部品 (ButtonAction / AxisInput) が引数で受け取って使う。
///   - 状態を持たない (前フレームとの比較は使う側＝ButtonAction の仕事)。
///   - Input* は借りるだけで所有しない。
/// </summary>
class InputReader
{
public:
	/// <param name="input">入力デバイス。所有しない (nullptr 可: 常に無入力扱い)</param>
	explicit InputReader(Input* input) : input_(input) {}

	/// パッドが繋がっているか (デバッグ表示や操作ヒントの切替に使う)
	bool IsPadConnected() const;

	///========================
	// 押しているか (未割り当ては false)
	///========================

	bool PushKey(BYTE key) const;
	bool PushPadButton(PadButton button) const;
	bool PushMouseButton(MouseButton button) const;
	bool PushPadTrigger(PadTrigger trigger) const;

	///========================
	// アナログ値 (未割り当て/未接続は 0)
	///========================

	/// スティック1軸の傾き (-1.0〜1.0、デッドゾーン処理済み。Yは上が正)
	float GetStick(StickAxis axis) const;

	/// マウスの1フレームの移動量[px] (画面座標なのでYは下が正)
	float GetMouseDelta(MouseAxis axis) const;

	/// マウスカーソルの位置[px] (クライアント座標)
	float GetMousePosition(MouseAxis axis) const;

	/// このフレームにマウスが動いたか
	/// (マウスとスティックを同じ照準に併用するとき、どちらを優先するかの判定に使う)
	bool IsMouseMoved() const;

private:
	// 軸の中立値
	static constexpr float kNeutral_ = 0.0f;

	// 所有はしない
	Input* input_ = nullptr;
};

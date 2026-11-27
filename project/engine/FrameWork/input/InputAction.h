#pragma once
#include "InputReader.h"

//======================================================
// 入力の部品（コンポーネント）
//------------------------------------------------------
// 「1つの操作」＝ ButtonAction、「1本の軸」＝ AxisInput。
// シーン入力クラスはこれを“必要な数だけ持つ”ことで組み立てる（has-a）。
// 継承で操作を増やさないので、タイトルに pause、ゲームに decide といった
// 使わない操作を引き継いでしまうことがない。
//======================================================

/// <summary>
/// ボタン操作1つ分の割り当て。使わない項目は未割り当てのままにする。
/// 複数指定した場合はその OR（キーでもパッドでも同じ操作になる）。
/// </summary>
struct ButtonBinding {
	BYTE key = kNoKey;                          // キーボード (DIK_*)
	BYTE altKey = kNoKey;                       // キーボードの副割り当て (W と ↑ の両対応など)
	PadButton padButton = PadButton::None;      // パッドのボタン
	MouseButton mouseButton = MouseButton::None;// マウスボタン
	PadTrigger padTrigger = PadTrigger::None;   // LT/RT をボタン扱いする場合
};

/// <summary>
/// 1本の軸の割り当て。
/// キーボードは「倒し切り」(±1)、スティックは傾きがそのまま入り、合計を -1〜1 に収める。
/// alt 側は副割り当て（WASD に加えて十字キーも効かせたい場合など）。
/// </summary>
struct AxisBinding {
	BYTE negativeKey = kNoKey;      // 負方向 (左/下)
	BYTE positiveKey = kNoKey;      // 正方向 (右/上)
	BYTE negativeAltKey = kNoKey;   // 負方向の副割り当て
	BYTE positiveAltKey = kNoKey;   // 正方向の副割り当て
	StickAxis stick = StickAxis::None;
};

/// <summary>
/// ボタン操作1つ分のコンポーネント。
///
/// 割り当てられた全デバイスの OR を取り、「押しっぱなし」と「押した瞬間」を自分で保持する。
/// 立ち上がり検出をここに閉じ込めるので、使う側が prevPressed / xxxLock_ のような
/// フラグを各所で持つ必要がなくなる。
///
/// 使い方: 毎フレーム Update() を1回 → IsPressed() / IsTriggered() を見る。
/// </summary>
class ButtonAction
{
public:
	explicit ButtonAction(const ButtonBinding& binding) : binding_(binding) {}

	/// <summary>今フレームの状態を取る（シーン入力の Update から呼ぶ）</summary>
	void Update(const InputReader& reader);

	/// 押しっぱなし
	bool IsPressed() const { return pressed_; }
	/// 押した瞬間（立ち上がり）
	bool IsTriggered() const { return triggered_; }

	/// 割り当ての差し替え（キーコンフィグ用）
	void SetBinding(const ButtonBinding& binding) { binding_ = binding; }
	const ButtonBinding& GetBinding() const { return binding_; }

private:
	ButtonBinding binding_{};

	bool pressed_ = false;
	bool previousPressed_ = false;
	bool triggered_ = false;

	// 1フレーム目は立ち上がりを出さない。
	// (シーン切替の瞬間に前シーンから押しっぱなしのボタンが暴発するのを防ぐ)
	bool initialized_ = false;
};

/// <summary>
/// 視点入力1軸分の割り当て。
///
/// 移動軸 (AxisBinding) と分ける理由:
///   移動は「どれだけ倒しているか」(-1〜1 の状態)だが、
///   視点は「このフレームでどれだけ動かしたか」(変化量) で、
///   マウスは移動量[px]、スティックは傾き×速度、と単位も感度も違う。
///   -1〜1 に正規化すると感度を表現できないため、別の部品にしてある。
/// </summary>
struct LookBinding {
	// キーボード：押している間、一定速度で動かす
	BYTE negativeKey = kNoKey;
	BYTE positiveKey = kNoKey;
	float keySpeed = 0.0f;           // 1フレームあたりの変化量

	// マウス：1フレームの移動量[px] × 感度
	MouseAxis mouseAxis = MouseAxis::None;
	float mouseSensitivity = 0.0f;   // [px] → 変化量

	// スティック：傾き × 速度
	StickAxis stick = StickAxis::None;
	float padSpeed = 0.0f;           // 倒し切りで1フレームあたりの変化量
	bool invertStick = false;        // マウスと向きを揃えたいとき (スティックは上が正)
};

/// <summary>
/// 視点入力1軸分の今フレームの変化量。デバイス系統を分けて渡す。
///
/// 分ける理由: 読む側 (カメラ) の扱いが違う。
///   キーボードは一定速度の旋回なのでそのまま使い、
///   マウス/スティックは演出中に止めたり鈍らせたり (感度スケール) したい。
///   どう混ぜるかは読む側の方針なので、入力側では決めない。
/// </summary>
struct LookAxisValue {
	float keyDelta = 0.0f;      // キーボードぶん
	float pointerDelta = 0.0f;  // マウス＋スティックぶん (感度込み)
};

/// <summary>
/// 視点入力1軸分のコンポーネント。
/// 毎フレーム Update() → GetValue() で今フレームの変化量を取る。
/// </summary>
class LookAxis
{
public:
	explicit LookAxis(const LookBinding& binding) : binding_(binding) {}

	void Update(const InputReader& reader);

	LookAxisValue GetValue() const { return value_; }

	/// 感度の差し替え（オプション画面用）
	void SetBinding(const LookBinding& binding) { binding_ = binding; }
	const LookBinding& GetBinding() const { return binding_; }

private:
	// スティックの向きを反転するときの係数
	static constexpr float kInvertSign_ = -1.0f;
	static constexpr float kNormalSign_ = 1.0f;

	LookBinding binding_{};
	LookAxisValue value_{};
};

/// <summary>
/// 照準（画面上の1軸ポインタ）の割り当て。
///
/// 視点 (LookBinding) と分ける理由:
///   視点は「変化量」だけを足し込むが、照準はマウスが絶対位置を持っている。
///   マウスを動かした瞬間はカーソルの座標へ飛ばし、動いていない間だけ
///   スティックの傾きで動かす（スティックに絶対位置は無いため）という混ぜ方が必要。
/// </summary>
struct AimBinding {
	// マウス：この軸のカーソル座標へ直接合わせる
	MouseAxis mouseAxis = MouseAxis::None;

	// スティック：傾き × 速度[px/秒] を足し込む
	StickAxis stick = StickAxis::None;
	float padSpeed = 0.0f;
	bool invertStick = false;   // スティックは上が正、画面Yは下が正

	// 可動範囲[px]（画面内に収める）
	float min = 0.0f;
	float max = 0.0f;
};

/// <summary>
/// 照準1軸分のコンポーネント。
///
/// 状態を持たない（＝位置は使う側が持つ）:
///   照準の位置は「斬撃線がどこにあるか」というゲーム側の状態で、
///   描画や命中判定にも使われる。入力側で二重に持たない。
///   使う側は「今動かしたい軸」についてだけ Move() を呼べばよく、
///   どの軸を動かすかの都合（斬るたびに縦横が入れ替わる等）を入力側が知る必要もない。
/// </summary>
class AimAxis
{
public:
	explicit AimAxis(const AimBinding& binding) : binding_(binding) {}

	/// <summary>
	/// 現在位置を、今フレームの入力で動かした位置を返す[px]。
	/// マウスが動いたフレームはカーソル座標へ飛び、それ以外はスティックの傾きで動かす。
	/// </summary>
	/// <param name="current">今の位置[px]</param>
	/// <param name="deltaTime">経過時間[秒]（スロー演出に引っ張られたくないなら unscaled を渡す）</param>
	float Move(const InputReader& reader, float current, float deltaTime) const;

	/// 可動範囲の中央[px]（演出開始時に線を中央へ置くときに使う）
	float GetCenter() const;

	/// 感度の差し替え（オプション画面用）
	void SetBinding(const AimBinding& binding) { binding_ = binding; }
	const AimBinding& GetBinding() const { return binding_; }

private:
	// スティックの向きを反転するときの係数
	static constexpr float kInvertSign_ = -1.0f;
	static constexpr float kNormalSign_ = 1.0f;
	// 可動範囲の中央を出すための係数
	static constexpr float kHalf_ = 0.5f;

	AimBinding binding_{};
};

/// <summary>
/// 軸1本分のコンポーネント。キーボードとスティックを合成して -1.0〜1.0 で返す。
/// </summary>
class AxisInput
{
public:
	explicit AxisInput(const AxisBinding& binding) : binding_(binding) {}

	/// <summary>今フレームの値を取る（シーン入力の Update から呼ぶ）</summary>
	void Update(const InputReader& reader);

	float GetValue() const { return value_; }

	/// 割り当ての差し替え（キーコンフィグ用）
	void SetBinding(const AxisBinding& binding) { binding_ = binding; }
	const AxisBinding& GetBinding() const { return binding_; }

private:
	// 軸の値域
	static constexpr float kAxisMin_ = -1.0f;
	static constexpr float kAxisMax_ = 1.0f;
	static constexpr float kAxisNeutral_ = 0.0f;
	// キーボードはアナログ量が無いので倒し切り扱い
	static constexpr float kKeyAxisAmount_ = 1.0f;

	static float ClampAxis(float value);

	AxisBinding binding_{};
	float value_ = kAxisNeutral_;
};

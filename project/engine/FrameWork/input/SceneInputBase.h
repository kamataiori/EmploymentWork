#pragma once
#include "Input.h"

/// <summary>
/// シーン単位の「入力の意味」をまとめる基底クラス。
///
/// 目的:
///   キーボード / ゲームパッド という“デバイスの違い”をここで吸収し、
///   シーンやキャラには moveX / decide / pause といった
///   “ゲーム上の意味”だけを渡す。
///   (Player が移動入力を直接読み、Scene がポーズ入力を直接読む…という
///    入力処理の散らばりを無くすための層)
///
/// 設計:
///   - シングルトンにしない。シーンが unique_ptr で 1 つ持つ。
///   - Input* は注入する (所有はしない)。差し替え/テストができるようにするため。
///   - 派生クラスは Update() の中で、下の Read○○ ヘルパーを使って
///     自分のフィールド (moveX_ など) を埋めるだけにする。
///   - シーンは「毎フレーム先頭で Update() → あとは Getter を見るだけ」。
///     同じフレーム内で何度 Getter を見ても同じ値になる。
///
/// 使い方 (派生側):
///   void TitleSceneInput::Update() {
///       moveX_  = ReadAxisX(kLeftKey_, kRightKey_);
///       decide_ = TriggerAny(kDecideKey_, kDecideButton_);
///   }
/// </summary>
class SceneInputBase
{
public:
	/// <param name="input">入力デバイス。所有はしない (nullptr 可: その場合は常に無入力扱い)</param>
	explicit SceneInputBase(Input* input) : input_(input) {}
	virtual ~SceneInputBase() = default;

	SceneInputBase(const SceneInputBase&) = delete;
	SceneInputBase& operator=(const SceneInputBase&) = delete;

	/// <summary>
	/// 入力のスナップショットを取る。シーンの Update の先頭で 1 回だけ呼ぶ。
	/// </summary>
	virtual void Update() = 0;

protected:
	///========================
	// 軸の定数
	///========================

	// 軸の値域 (キーボードもパッドもこの範囲に正規化して返す)
	static constexpr float kAxisMin_ = -1.0f;
	static constexpr float kAxisMax_ = 1.0f;
	// 軸の中立 (入力なし)
	static constexpr float kAxisNeutral_ = 0.0f;
	// キーボードは「倒し切り」として扱う (アナログ量が無いため)
	static constexpr float kKeyAxisAmount_ = 1.0f;

	///========================
	// 入力読み取りヘルパー
	///========================

	/// <summary>
	/// キー2つ＋左スティックX を 1 本の横軸にまとめる。負方向キーで -1、正方向キーで +1。
	/// </summary>
	float ReadAxisX(BYTE negativeKey, BYTE positiveKey) const;

	/// <summary>
	/// キー2つ＋左スティックY を 1 本の縦軸にまとめる。上が +1 (スティックの生値と同じ向き)。
	/// </summary>
	float ReadAxisY(BYTE negativeKey, BYTE positiveKey) const;

	/// <summary>
	/// キーのみの軸 (WASD と十字キーの両対応など、1 本の軸に複数割り当てを足すとき用)。
	/// </summary>
	float ReadKeyAxis(BYTE negativeKey, BYTE positiveKey) const;

	/// <summary>キーかパッドボタンのどちらかを「押している」</summary>
	bool PushAny(BYTE key, PadButton button) const;

	/// <summary>キーかパッドボタンのどちらかを「押した瞬間」</summary>
	bool TriggerAny(BYTE key, PadButton button) const;

	/// <summary>2 系統の割り当てを合成して値域に収める (ReadAxis 同士を足すとき用)</summary>
	static float ClampAxis(float value);

	/// <summary>入力デバイス (nullptr の可能性あり)</summary>
	Input* GetInput() const { return input_; }

private:
	// 未接続/未注入でも安全に 0 を返すためのラッパー
	float ReadLeftStickX() const;
	float ReadLeftStickY() const;

	// 所有はしない
	Input* input_ = nullptr;
};

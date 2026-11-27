#include "InputAction.h"

void ButtonAction::Update(const InputReader& reader)
{
	// 割り当てられたデバイスのどれかが押されていれば「押している」
	pressed_ =
		reader.PushKey(binding_.key)
		|| reader.PushKey(binding_.altKey)
		|| reader.PushPadButton(binding_.padButton)
		|| reader.PushMouseButton(binding_.mouseButton)
		|| reader.PushPadTrigger(binding_.padTrigger);

	if (!initialized_) {
		// 初回は今の状態を「前フレーム」として取り込むだけ（＝立ち上がりを出さない）
		initialized_ = true;
		previousPressed_ = pressed_;
	}

	triggered_ = pressed_ && !previousPressed_;
	previousPressed_ = pressed_;
}

void LookAxis::Update(const InputReader& reader)
{
	// キーボード：押している間、一定速度
	float keyDelta = 0.0f;
	if (reader.PushKey(binding_.negativeKey)) keyDelta -= binding_.keySpeed;
	if (reader.PushKey(binding_.positiveKey)) keyDelta += binding_.keySpeed;

	// マウス：移動量[px] × 感度
	float pointerDelta = reader.GetMouseDelta(binding_.mouseAxis) * binding_.mouseSensitivity;

	// スティック：傾き × 速度（必要なら向きを反転してマウスと揃える）
	const float stickSign = binding_.invertStick ? kInvertSign_ : kNormalSign_;
	pointerDelta += reader.GetStick(binding_.stick) * binding_.padSpeed * stickSign;

	value_.keyDelta = keyDelta;
	value_.pointerDelta = pointerDelta;
}

float AimAxis::Move(const InputReader& reader, float current, float deltaTime) const
{
	float position = current;

	if (reader.IsMouseMoved()) {
		// マウスは絶対位置を持つので、動かしたフレームはカーソルへ合わせる
		position = reader.GetMousePosition(binding_.mouseAxis);
	}
	else {
		// スティックは絶対位置が無いので、傾きぶんを足し込む
		const float stickSign = binding_.invertStick ? kInvertSign_ : kNormalSign_;
		position += reader.GetStick(binding_.stick) * binding_.padSpeed * stickSign * deltaTime;
	}

	// 画面内に収める
	if (position < binding_.min) position = binding_.min;
	if (position > binding_.max) position = binding_.max;
	return position;
}

float AimAxis::GetCenter() const
{
	return (binding_.min + binding_.max) * kHalf_;
}

void AxisInput::Update(const InputReader& reader)
{
	float axis = kAxisNeutral_;

	// 主割り当て（WASD など）
	if (reader.PushKey(binding_.negativeKey)) axis -= kKeyAxisAmount_;
	if (reader.PushKey(binding_.positiveKey)) axis += kKeyAxisAmount_;

	// 副割り当て（十字キーなど。主割り当てと同じ軸に足す）
	if (reader.PushKey(binding_.negativeAltKey)) axis -= kKeyAxisAmount_;
	if (reader.PushKey(binding_.positiveAltKey)) axis += kKeyAxisAmount_;

	// スティック（デッドゾーン処理済みの傾きがそのまま入る）
	axis += reader.GetStick(binding_.stick);

	value_ = ClampAxis(axis);
}

float AxisInput::ClampAxis(float value)
{
	if (value < kAxisMin_) return kAxisMin_;
	if (value > kAxisMax_) return kAxisMax_;
	return value;
}

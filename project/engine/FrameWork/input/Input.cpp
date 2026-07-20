#include "Input.h"
#include <algorithm>
#include <cmath>

namespace {
	/// スティック1本分の生値(-32768〜32767)を、デッドゾーンを抜いた -1.0〜1.0 へ直す。
	/// 軸ごとに切るとナナメ入力が歪むので、2軸の長さで判定してから正規化する。
	void ApplyStickDeadzone(SHORT rawX, SHORT rawY, SHORT deadzone, float& outX, float& outY)
	{
		float x = static_cast<float>(rawX);
		float y = static_cast<float>(rawY);
		const float length = std::sqrt(x * x + y * y);

		if (length <= static_cast<float>(deadzone)) {
			outX = 0.0f;
			outY = 0.0f;
			return;
		}

		// 生値の最大。-32768 側が1だけ大きいが、扱いを揃えるため 32767 を上限にする
		constexpr float kMaxMagnitude = 32767.0f;

		// デッドゾーンの外側を 0.0〜1.0 へ引き直す（境界でカクつかせないため）
		const float clamped = (std::min)(length, kMaxMagnitude);
		const float scaled = (clamped - deadzone) / (kMaxMagnitude - deadzone);

		outX = (x / length) * scaled;
		outY = (y / length) * scaled;
	}

	/// トリガーの生値(0〜255)を、デッドゾーンを抜いた 0.0〜1.0 へ直す
	float ApplyTriggerDeadzone(BYTE raw)
	{
		if (raw <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
			return 0.0f;
		}
		constexpr float kMaxTrigger = 255.0f;
		return (static_cast<float>(raw) - XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			/ (kMaxTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	}
}

Input* Input::instance = nullptr;

Input* Input::GetInstance()
{
	if (!instance) {
		instance = new Input();
	}
	return instance;
}

void Input::Finalize()
{
	delete instance;
	instance = nullptr;
}

void Input::Initialize(WinApp* winApp)
{
	//借りてきたWinAppのインスタンスを記録
	this->winApp_ = winApp;

	//DirectInputのインスタンス生成
	//Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
	HRESULT result = DirectInput8Create(winApp->GetInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));
	//キーボードデバイス生成
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));
	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	directInput->CreateDevice(GUID_SysMouse, &mouse_, nullptr);
	mouse_->SetDataFormat(&c_dfDIMouse2);
	mouse_->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	mouse_->Acquire();
}

void Input::Update()
{
	HRESULT result;

	// パッドはここで更新する。この下のマウス処理は取得に失敗すると return するので、
	// 後ろに置くとフォーカスを失った時にパッドまで止まってしまう。
	UpdatePad();

	//前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));

	//キーボード情報の取得開始
	result = keyboard->Acquire();
	////全キーの入力情報を取得する
	//BYTE key[256] = {};
	result = keyboard->GetDeviceState(sizeof(key), key);

	// 前フレームの状態保存
	mouseStatePrev_ = mouseState_;
	ZeroMemory(&mouseState_, sizeof(mouseState_));

	// 状態取得
	HRESULT hr = mouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &mouseState_);
	if (FAILED(hr)) {
		mouse_->Acquire(); // フォーカス失った場合など
		return;
	}

	// ボタン：0=左, 1=右, 2=中, 3,4=サイド
	// ホイール
	wheel_ = static_cast<int>(mouseState_.lZ);
	// 移動量（相対）
	mouseDelta_.x = mouseState_.lX;
	mouseDelta_.y = mouseState_.lY;
}

bool Input::PushKey(BYTE keyNumber)
{
	//指定キーを押していればtrueを返す
	if (key[keyNumber])
	{
		return true;
	}

	//そうでなければfalseを返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	/*if (keyPre[keyNumber])
	{
		return true;
	}

	return false;*/

	return (key[keyNumber] && !keyPre[keyNumber]);
}

bool Input::PushMouseButton(int button) {
	return (mouseState_.rgbButtons[button] & 0x80);
}

bool Input::TriggerMouseButton(int button) {
	return (mouseState_.rgbButtons[button] & 0x80) && !(mouseStatePrev_.rgbButtons[button] & 0x80);
}

int Input::GetMouseWheel() const {
	return wheel_;
}

POINT Input::GetMouseDelta() const {
	return mouseDelta_;
}

POINT Input::GetMousePosition() const
{
	POINT p{};
	GetCursorPos(&p);
	ScreenToClient(winApp_->GetHwnd(), &p);
	return p;
}

///========================
// ゲームパッド（XInput）
///========================

void Input::UpdatePad()
{
	padStatePrev_ = padState_;

	// 未接続のパッドへの XInputGetState は重い。毎フレーム叩かず間隔を空けて再確認する。
	if (!padConnected_ && padPollCooldown_ > 0) {
		--padPollCooldown_;
		return;
	}

	XINPUT_STATE state{};
	const DWORD result = XInputGetState(kPadIndex_, &state);

	if (result != ERROR_SUCCESS) {
		// 未接続。抜かれた瞬間に入力が残らないよう、値を全部落とす
		padConnected_ = false;
		padPollCooldown_ = kPadPollInterval_;
		ZeroMemory(&padState_, sizeof(padState_));
		leftStickX_ = leftStickY_ = 0.0f;
		rightStickX_ = rightStickY_ = 0.0f;
		leftTrigger_ = rightTrigger_ = 0.0f;
		return;
	}

	padConnected_ = true;
	padState_ = state;

	ApplyStickDeadzone(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY,
		XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, leftStickX_, leftStickY_);
	ApplyStickDeadzone(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY,
		XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rightStickX_, rightStickY_);

	leftTrigger_ = ApplyTriggerDeadzone(state.Gamepad.bLeftTrigger);
	rightTrigger_ = ApplyTriggerDeadzone(state.Gamepad.bRightTrigger);
}

bool Input::PushButton(PadButton button) const
{
	const WORD mask = static_cast<WORD>(button);
	return (padState_.Gamepad.wButtons & mask) != 0;
}

bool Input::TriggerButton(PadButton button) const
{
	const WORD mask = static_cast<WORD>(button);
	return (padState_.Gamepad.wButtons & mask) != 0
		&& (padStatePrev_.Gamepad.wButtons & mask) == 0;
}

bool Input::PushLeftTrigger() const
{
	return leftTrigger_ >= kTriggerPressThreshold_;
}

bool Input::PushRightTrigger() const
{
	return rightTrigger_ >= kTriggerPressThreshold_;
}

bool Input::TriggerLeftTrigger() const
{
	const float prev = ApplyTriggerDeadzone(padStatePrev_.Gamepad.bLeftTrigger);
	return leftTrigger_ >= kTriggerPressThreshold_ && prev < kTriggerPressThreshold_;
}

bool Input::TriggerRightTrigger() const
{
	const float prev = ApplyTriggerDeadzone(padStatePrev_.Gamepad.bRightTrigger);
	return rightTrigger_ >= kTriggerPressThreshold_ && prev < kTriggerPressThreshold_;
}

void Input::SetVibration(float leftMotor, float rightMotor)
{
	if (!padConnected_) return;

	constexpr float kMaxMotorSpeed = 65535.0f;
	XINPUT_VIBRATION vibration{};
	vibration.wLeftMotorSpeed =
		static_cast<WORD>(std::clamp(leftMotor, 0.0f, 1.0f) * kMaxMotorSpeed);
	vibration.wRightMotorSpeed =
		static_cast<WORD>(std::clamp(rightMotor, 0.0f, 1.0f) * kMaxMotorSpeed);
	XInputSetState(kPadIndex_, &vibration);
}

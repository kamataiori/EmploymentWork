#include "Input.h"

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
	if (keyPre[keyNumber])
	{
		return true;
	}

	return false;
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

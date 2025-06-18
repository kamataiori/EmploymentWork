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

	// マウス移動
	mousePosPrev_ = mousePos_;
	GetCursorPos(&mousePos_);
	ScreenToClient(winApp_->GetHwnd(), &mousePos_);

	mouseMove_.x = mousePos_.x - mousePosPrev_.x;
	mouseMove_.y = mousePos_.y - mousePosPrev_.y;

	// マウスボタン
	for (int i = 0; i < 5; ++i) {
		mouseButtonPrev_[i] = mouseButton_[i];
		mouseButton_[i] = (GetAsyncKeyState(VK_LBUTTON + i) & 0x8000) != 0;
	}

	// マウスホイール（短期間ならこれで十分）
	wheelPrev_ = wheel_;
	wheel_ = 0;
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_MOUSEWHEEL) {
			wheel_ += GET_WHEEL_DELTA_WPARAM(msg.wParam);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
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
	if (button < 0 || button >= 5) return false;
	return mouseButton_[button];
}

bool Input::TriggerMouseButton(int button) {
	if (button < 0 || button >= 5) return false;
	return mouseButton_[button] && !mouseButtonPrev_[button];
}

int Input::GetMouseFlickX() const {
	if (mouseMove_.x > 2) return 1;
	if (mouseMove_.x < -2) return -1;
	return 0;
}

int Input::GetMouseFlickY() const {
	if (mouseMove_.y > 2) return 1;
	if (mouseMove_.y < -2) return -1;
	return 0;
}


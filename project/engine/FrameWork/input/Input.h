#pragma once
#define DIRECTINPUT_VERSION    0x0800 //DirectInputのバージョン指定
#include <wrl.h>
#include <cassert>
#include <dinput.h>
#include <Windows.h>
#include <Xinput.h>
#include "WinApp.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"xinput.lib")

/// <summary>
/// ゲームパッドのボタン。値は XInput の XINPUT_GAMEPAD_* をそのまま使う。
/// （PushButton / TriggerButton へ渡す）
/// </summary>
enum class PadButton : WORD {
	Up        = XINPUT_GAMEPAD_DPAD_UP,
	Down      = XINPUT_GAMEPAD_DPAD_DOWN,
	Left      = XINPUT_GAMEPAD_DPAD_LEFT,
	Right     = XINPUT_GAMEPAD_DPAD_RIGHT,
	Start     = XINPUT_GAMEPAD_START,
	Back      = XINPUT_GAMEPAD_BACK,
	LeftThumb = XINPUT_GAMEPAD_LEFT_THUMB,
	RightThumb= XINPUT_GAMEPAD_RIGHT_THUMB,
	LB        = XINPUT_GAMEPAD_LEFT_SHOULDER,
	RB        = XINPUT_GAMEPAD_RIGHT_SHOULDER,
	A         = XINPUT_GAMEPAD_A,
	B         = XINPUT_GAMEPAD_B,
	X         = XINPUT_GAMEPAD_X,
	Y         = XINPUT_GAMEPAD_Y,
};

//入力
class Input
{
public:
	static Input* instance;

	// インスタンスを取得するシングルトンメソッド
	static Input* GetInstance();

	// プライベートコンストラクタ
	Input() = default;

	// コピーコンストラクタおよび代入演算子を削除
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

public:

	//namespace省略
	template <class T> using Comptr = Microsoft::WRL::ComPtr<T>;

	///========================
	// メンバ関数
	///========================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// キーの押下をチェック
	/// </summary>
	/// <param name="keyNumber">キー番号( DIK_0 等)</param>
	/// <returns>押されているか</returns>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号( DIK_0 等)</param>
	/// <returns>トリガーか</returns>
	bool TriggerKey(BYTE keyNumber);

	// ボタン押下・トリガー（0:左, 1:右, 2:中, 3:サイド1, 4:サイド2）
	bool PushMouseButton(int button);     // 押してる
	bool TriggerMouseButton(int button);  // 押した瞬間

	int GetMouseWheel() const;
	POINT GetMouseDelta() const;

	// マウス座標（クライアント座標）取得
	POINT GetMousePosition() const;

	///========================
	// ゲームパッド（XInput）
	//   コントローラ1台のみ対応。抜き差しは自動で追従する。
	///========================

	/// パッドが繋がっているか（未接続時、以下の取得はすべて0/falseを返す）
	bool IsPadConnected() const { return padConnected_; }

	/// ボタンを押している / 押した瞬間
	bool PushButton(PadButton button) const;
	bool TriggerButton(PadButton button) const;

	/// スティックの傾き。デッドゾーン処理済みで、各軸 -1.0〜1.0 に正規化される。
	/// Yは上が正（XInputの生値と同じ向き）。
	float GetLeftStickX() const  { return leftStickX_; }
	float GetLeftStickY() const  { return leftStickY_; }
	float GetRightStickX() const { return rightStickX_; }
	float GetRightStickY() const { return rightStickY_; }

	/// アナログトリガーの踏み込み量（0.0〜1.0）。デッドゾーン処理済み。
	float GetLeftTrigger() const  { return leftTrigger_; }
	float GetRightTrigger() const { return rightTrigger_; }

	/// トリガーを「ボタンとして」押している / 押した瞬間（しきい値以上で押下扱い）
	bool PushLeftTrigger() const;
	bool PushRightTrigger() const;
	bool TriggerLeftTrigger() const;
	bool TriggerRightTrigger() const;

	/// 振動（0.0〜1.0）。0を渡すと停止する。
	void SetVibration(float leftMotor, float rightMotor);

private:
	/// パッドの状態取得。未接続なら値をゼロクリアする
	void UpdatePad();

	///========================
	// メンバ変数
	///========================

	/// キーボードのデバイス
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard;

	//DirectInputのインスタンス
	Microsoft::WRL::ComPtr<IDirectInput8> directInput;

	//全キーの状態
	BYTE key[256] = {};
	//前回の全キーの状態
	BYTE keyPre[256] = {};

	HRESULT result = {};

	//WindowsAPI
	WinApp* winApp_ = nullptr;

	// マウスの状態
	Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

	DIMOUSESTATE2 mouseState_{};       // 現在のマウス状態
	DIMOUSESTATE2 mouseStatePrev_{};   // 前フレームの状態

	int wheel_ = 0;                    // ホイール回転量
	POINT mouseDelta_ = {};            // 移動量

	///========================
	// ゲームパッド（XInput）
	///========================
	XINPUT_STATE padState_{};      // 現在のパッド状態
	XINPUT_STATE padStatePrev_{};  // 前フレームの状態
	bool padConnected_ = false;

	// デッドゾーン処理済みの値（Update で作る）
	float leftStickX_ = 0.0f, leftStickY_ = 0.0f;
	float rightStickX_ = 0.0f, rightStickY_ = 0.0f;
	float leftTrigger_ = 0.0f, rightTrigger_ = 0.0f;

	// 未接続のパッドを毎フレーム問い合わせると重いので、間隔を空けて再確認する
	int padPollCooldown_ = 0;
	static constexpr int kPadPollInterval_ = 60; // 未接続時の再確認間隔（フレーム）

	// 使うのは1台目のみ
	static constexpr DWORD kPadIndex_ = 0;
	// トリガーをボタン扱いするときのしきい値（0.0〜1.0）
	static constexpr float kTriggerPressThreshold_ = 0.5f;
};


#pragma once
#define DIRECTINPUT_VERSION    0x0800 //DirectInputのバージョン指定
#include <wrl.h>
#include <cassert>
#include <dinput.h>
#include <Windows.h>
#include "WinApp.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")



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

private:

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

};


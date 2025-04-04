#pragma once
#include <windows.h>
#include <cstdint>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


//WindowsAPI
class WinApp
{
public:

	///===========================
	// 静的メンバ関数
	///===========================

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


	///===========================
	// 定数
	///===========================

	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;


	///===========================
	// メンバ関数
	///===========================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void UpDate();

	/// <summary>
	/// ゲッター
	/// </summary>
	/// <returns>hwnd</returns>
	HWND GetHwnd() const { return hwnd; }

	HINSTANCE GetInstance() const { return wc.hInstance; }

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// メッセージの終了処理
	/// </summary>
	/// <returns></returns>
	bool ProcessMesage();


private:

	///===========================
	// メンバ変数
	///===========================

	//ウィンドウハンドル
	HWND hwnd = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc{};

};


#pragma once
#include "WinApp.h"
#include <memory>
#include "DirectXCommon.h"

class ImGuiManager
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	// WinAppの初期化
	std::unique_ptr<WinApp> winApp_ = nullptr;
	// DirectXの初期化
	DirectXCommon* dxCommon_ = nullptr;
	// SRV用でスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
	uint32_t srvIndex;
};


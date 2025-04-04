#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "StringUtility.h"
#include "WinApp.h"
#include <chrono>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <Vector4.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib, "dxgi.lib") 
#pragma comment(lib, "dxcompiler.lib") 


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


class DirectXCommon
{
public:
	static DirectXCommon* instance;

	// インスタンスを取得するシングルトンメソッド
	static DirectXCommon* GetInstance();

	// プライベートコンストラクタ
	DirectXCommon() = default;

	// コピーコンストラクタおよび代入演算子を削除
	DirectXCommon(const DirectXCommon&) = delete;
	DirectXCommon& operator=(const DirectXCommon&) = delete;

public:

	///===========================
	// メンバ関数
	///===========================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// デバイスの初期化
	/// </summary>
	void Device();

	/// <summary>
	/// コマンド関連の初期化
	/// </summary>
	void Command();

	/// <summary>
	/// スワップチェーンの生成
	/// </summary>
	void SwapChain();

	/// <summary>
	/// 深度バッファの生成
	/// </summary>
	void DepthBuffer();

	/// <summary>
	/// 各種ディスクリプタヒープの生成
	/// </summary>
	void DescriptorHeap();

	/// <summary>
	/// レンダーターゲットビューの初期化
	/// </summary>
	void RenderTargetView();

	/// <summary>
	/// 深度ステンシルビューの初期化
	/// </summary>
	void DepthStencilView();

	/// <summary>
	/// フェンスの生成
	/// </summary>
	void Fence();

	/// <summary>
	/// ビューポート矩形の初期化
	/// </summary>
	void ViewportRectangle();

	/// <summary>
	/// シザリング矩形の初期化
	/// </summary>
	void ScissoringRectangle();

	/// <summary>
	/// DXCコンパイラの生成
	/// </summary>
	void DXCCompiler();

	/// <summary>
	/// ImGuiの初期化
	/// </summary>
	void ImGui();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// ディスクリプタヒープの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 指定番号のCPUディスクリプタハンドルを取得する
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// 指定番号のGPUディスクリプタハンドルを取得する
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);


	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);


	/// <summary>
	/// シェーダーのコンパイル関数
	/// </summary>
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath, const wchar_t* profile);



	////====================リソース生成関数====================////

	/// <summary>
	/// バッファリソースの生成関数
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);


	//====テクスチャを読む====//

	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	/// <summary>
	/// テクスチャリソースの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	/// <summary>
	/// レンダーテクスチャの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);

	/// <summary>
	/// テクスチャデータの転送
	/// </summary>
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);



	////===============描画系===============////

	/// <summary>
	/// OffscreenRenderingのPreDraw
	/// </summary>
	void PreDrawForRenderTexture();

	/// <summary>
	/// OffscreenRenderingのPostDraw
	/// </summary>
	void PostDrawForRenderTexture();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// 描画後処理
	/// </summary>
	void PostDraw();


	////====================ゲッター関数====================////

	//==========device==========//

	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const { return device; }


	// CreateBufferResource 関数
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCreateBufferResource(size_t sizeInBytes);

	// バックバッファの数を取得
	size_t GetBackBufferCount() const {return swapChainDesc.BufferCount;}

	//==========dxcCompiler==========//

	/// <summary>
	/// dxcUtilsのゲッター
	/// </summary>
	IDxcUtils* GetDxcUtils() const { return dxcUtils; }

	/// <summary>
	/// dxcCompilerのゲッター
	/// </summary>
	IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler; }

	/// <summary>
	/// includeHandlerのゲッター
	/// </summary>
	IDxcIncludeHandler* GetIncludeHandler() const {return includeHandler;}

	/// <summary>
	/// swapChainのゲッター
	/// </summary>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapChain() const { return swapChain; }

	/// <summary>
	/// swapChainResourcesのゲッター
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> GetSwapChainResource(int index) const
	{
		if (index < 0 || index >= 2)
		{
			throw std::out_of_range("Index out of range for swapChainResources");
		}
		return swapChainResources[index];
	}

	/// <summary>
	/// commandListのゲッター
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return commandList; }

	/// <summary>
	/// dsvDescriptorHeapのゲッター
	/// </summary>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDsvDescriptorHeap() const { return dsvDescriptorHeap; }

	/// <summary>
	/// rtvHandlesのゲッター
	/// </summary>
	/// <param name="index"></param>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(int index) const
	{
		if (index < 0 || index >= 2)
		{
			throw std::out_of_range("Index out of range for rtvHandles");
		}
		return rtvHandles[index];
	}

	/// <summary>
	/// レンダーテクスチャのcpuハンドルの取得
	/// </summary>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTextureRTVHandle()
	{
		// RTVのディスクリプタヒープが存在するか確認
		if (!rtvDescriptorHeap)
		{
			throw std::runtime_error("RTV Descriptor Heap not initialized");
		}

		// ディスクリプタヒープの先頭ハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// 必要なオフセットを適用（ここでは例としてインデックス0を使用）
		handle.ptr += descriptorSizeRTV * 0; // インデックスを調整する場合はここを変更

		return handle;
	}


	/// <summary>
	/// commandQueueのゲッター
	/// </summary>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return commandQueue; }


	// Microsoft::WRL::ComPtr<ID3D12Fence> fence のゲッター
	Microsoft::WRL::ComPtr<ID3D12Fence> GetFence() const { return fence; }

	// uint64_t fenceValue のゲッター
	uint64_t GetFenceValue() const { return fenceValue; }

	// HANDLE fenceEvent のゲッター
	HANDLE GetFenceEvent() const { return fenceEvent; }

	/// <summary>
	/// commandAllocatorのゲッター
	/// </summary>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return commandAllocator; }


	////====================セッター関数====================////


	/// <summary>
	/// fenceValueのセッター
	/// </summary>
	/// <param name="value"></param>
	void SetFenceValue(uint64_t value) { fenceValue = value; }



	////最大SRV数(最大テクスチャ枚数)
	//static const uint32_t kMaxSRVCount;


private:

	///===========================
	// メンバ関数
	///===========================

	//FPS固定初期化
	void InitializeFixFPS();
	//FPS固定更新
	void UpdateFixFPS();
	//記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	void ShowFPS(); // ImGuiでFPSを表示する関数
	//std::chrono::steady_clock::time_point reference_ = std::chrono::steady_clock::now();
	//float fps_ = 0.0f; // 計算されたFPSを格納


	///===========================
	// メンバ変数
	///===========================

	//VertexResource生成関数
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;


	//DXGI1ファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	//コマンドアロケータの生成
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator{};
	//コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList{};
	//コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue{};

	//WindowsAPI
	WinApp* winApp = nullptr;

	//スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};


	//DescriptorSizeの取得
	//uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;


	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3]{};

	//RTV用のヒープディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;

	////SRV用のヒープディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;

	//DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;

	//描画先のRTVとDSVを指定
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
	//SRV用のディスクリプタヒープを指定
	ID3D12DescriptorHeap* descriptorHeaps[1];


	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStarHandle{};

	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };

	// オフスクリーンリソースの作成
	Microsoft::WRL::ComPtr<ID3D12Resource> offscreenResource = nullptr;

	//DescriptorHeapの作成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};


	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU{};



	//RTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource{};



	//初期値0でFenceを作る
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	//初期値0でFenceを作る
	uint64_t fenceValue = 0;
	//FenceのSignalを持つためのイベントを生成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	//ビューポート矩形
	D3D12_VIEWPORT viewport{};
	//シザー矩形
	D3D12_RECT scissorRect{};


	//dxcCompiler
	IDxcUtils* dxcUtils{};
	IDxcCompiler3* dxcCompiler{};
	IDxcIncludeHandler* includeHandler{};


	//リソースバリアで書き込み可能に変更
	D3D12_RESOURCE_BARRIER barrier{};

};


#include "ImGuiManager.h"
#include <externals/imgui/imgui_impl_win32.h>
#include <SrvManager.h>

void ImGuiManager::Initialize(WinApp* winApp, DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	// ImGuiのコンテキストを生成
	ImGui::CreateContext();
	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	// Win32用初期化
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	//// デスクリプタヒープの設定
	//D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//desc.NumDescriptors = 1;
	//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//// デスクリプタヒープの生成
	//HRESULT result = dxCommon_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
	//assert(SUCCEEDED(result));

	srvIndex = SrvManager::GetInstance()->Allocate();

	// DirectX12用初期化
	ImGui_ImplDX12_Init(
		dxCommon_->GetDevice().Get(),
		static_cast<int>(dxCommon_->GetBackBufferCount()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 
		SrvManager::GetInstance()->GetSrvDescriptorHeap().Get(),
		SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex),
		SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex)
	);

	// エディター同士をドッキング
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// フォント追加（日本語対応）
	// --- ✅ 可変フォント(NotoSansJP VariableFont) 読み込み ---
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 3;
	fontConfig.OversampleV = 3;
	fontConfig.PixelSnapH = true;

	// 可変ウェイトの範囲は 100〜900（400が通常）
	float fontSize = 18.0f;
	float fontWeight = 100.0f;  // 通常：Regular

	ImVector<ImWchar> glyphRanges;
	io.Fonts->GetGlyphRangesJapanese(); // 日本語対応

	io.Fonts->AddFontFromFileTTF(
		"Resources/fonts/NotoSansJP-VariableFont_wght.ttf",
		fontSize, &fontConfig,
		io.Fonts->GetGlyphRangesJapanese()
	);

	// デフォルトフォントに設定（省略可能）
	io.FontDefault = io.Fonts->Fonts.back();


}

void ImGuiManager::Finalize()
{
	// 後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// デスクリプタヒープを解放
	srvHeap_.Reset();
}

void ImGuiManager::Update()
{
	// ImGuiのフレーム開始を宣言
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::Draw()
{
	// デスクリプタヒープの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { SrvManager::GetInstance()->GetSrvDescriptorHeap().Get()};
	dxCommon_->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// ImGui の描画コマンドを積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon_->GetCommandList().Get());
}


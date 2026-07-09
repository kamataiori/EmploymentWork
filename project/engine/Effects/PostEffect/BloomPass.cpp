#include "BloomPass.h"
#include "DirectXCommon.h"
#include <WinApp.h>

//========================================================================
// 既定パラメータ
//========================================================================
namespace {
	constexpr float kDefaultThreshold = 0.8f;  // この輝度を超えた分をBloom化
	constexpr float kDefaultIntensity = 1.0f;  // Bloomの強さ
	constexpr uint32_t kBloomDownscale = 2;    // 内部RTの縮小率（半解像度）
}

//========================================================================
// BloomExtractPass
//========================================================================
void BloomExtractPass::OnInitialized()
{
	CreateConstantBuffer(cb_, mapped_);
	mapped_->threshold = kDefaultThreshold;
}
void BloomExtractPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	DrawFullscreen(commandList);
}

//========================================================================
// BloomBlurPass
//========================================================================
void BloomBlurPass::OnInitialized()
{
	CreateConstantBuffer(cb_, mapped_);
	mapped_->direction = { 1.0f, 0.0f };
}
void BloomBlurPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	DrawFullscreen(commandList);
}

//========================================================================
// BloomCompositePass
//========================================================================
void BloomCompositePass::OnInitialized()
{
	CreateConstantBuffer(cb_, mapped_);
	mapped_->intensity = kDefaultIntensity;
}
void BloomCompositePass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	DrawFullscreen(commandList);
}

//========================================================================
// BloomPass
//========================================================================
void BloomPass::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	// 内部パス生成
	extract_.Initialize(dxCommon_);
	blurH_.Initialize(dxCommon_);
	blurV_.Initialize(dxCommon_);
	composite_.Initialize(dxCommon_);
	copy_.Initialize(dxCommon_);

	// ブラー方向を固定（別インスタンス＝別CBなので混ざらない）
	blurH_.SetDirection({ 1.0f, 0.0f });
	blurV_.SetDirection({ 0.0f, 1.0f });

	// 既定パラメータ
	extract_.SetThreshold(kDefaultThreshold);
	composite_.SetIntensity(kDefaultIntensity);

	// 内部RT（半解像度）
	bloomWidth_ = WinApp::kClientWidth / kBloomDownscale;
	bloomHeight_ = WinApp::kClientHeight / kBloomDownscale;
	const Vector4 kClear{ 0.0f, 0.0f, 0.0f, 1.0f };

	extractRT_ = std::make_unique<RenderTarget>();
	extractRT_->Initialize(dxCommon_, bloomWidth_, bloomHeight_, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kClear);
	blurRT1_ = std::make_unique<RenderTarget>();
	blurRT1_->Initialize(dxCommon_, bloomWidth_, bloomHeight_, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kClear);
	blurRT2_ = std::make_unique<RenderTarget>();
	blurRT2_->Initialize(dxCommon_, bloomWidth_, bloomHeight_, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kClear);
}

void BloomPass::SetFinalOutput(DirectXCommon* dxCommon, RenderTarget* output)
{
	dxCommon_ = dxCommon;
	finalOutput_ = output;
}

void BloomPass::BindInternalRT(ID3D12GraphicsCommandList* commandList, RenderTarget* target)
{
	target->TransitionToRenderTarget(commandList);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = target->GetRtvHandle();
	commandList->OMSetRenderTargets(1, &rtv, false, nullptr); // 深度不要

	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(bloomWidth_);
	viewport.Height = static_cast<float>(bloomHeight_);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissor{};
	scissor.right = static_cast<LONG>(bloomWidth_);
	scissor.bottom = static_cast<LONG>(bloomHeight_);
	commandList->RSSetScissorRects(1, &scissor);
}

void BloomPass::RebindFinalOutput(ID3D12GraphicsCommandList* commandList)
{
	if (finalOutput_) {
		finalOutput_->TransitionToRenderTarget(commandList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = finalOutput_->GetRtvHandle();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();
		commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

		D3D12_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(WinApp::kClientWidth);
		viewport.Height = static_cast<float>(WinApp::kClientHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		commandList->RSSetViewports(1, &viewport);

		D3D12_RECT scissor{};
		scissor.right = static_cast<LONG>(WinApp::kClientWidth);
		scissor.bottom = static_cast<LONG>(WinApp::kClientHeight);
		commandList->RSSetScissorRects(1, &scissor);
	} else {
		dxCommon_->BindSwapChainRenderTarget();
	}
}

void BloomPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	// 1. 明るい部分を抽出（inputSrv → extractRT）
	BindInternalRT(commandList, extractRT_.get());
	extract_.Execute(commandList, inputSrvIndex);
	extractRT_->TransitionToShaderResource(commandList);

	// 2. 横／縦ブラーを反復適用（ping-pong）。反復するほど広く強いにじみになる。
	//    src を入力に H→blurRT1、V→blurRT2 とし、結果を次の src にして繰り返す。
	uint32_t src = extractRT_->GetSrvIndex();
	for (int i = 0; i < blurIterations_; ++i) {
		// 横ブラー（src → blurRT1）
		BindInternalRT(commandList, blurRT1_.get());
		blurH_.Execute(commandList, src);
		blurRT1_->TransitionToShaderResource(commandList);

		// 縦ブラー（blurRT1 → blurRT2）
		BindInternalRT(commandList, blurRT2_.get());
		blurV_.Execute(commandList, blurRT1_->GetSrvIndex());
		blurRT2_->TransitionToShaderResource(commandList);

		// 次の反復は今回の結果をさらにぼかす
		src = blurRT2_->GetSrvIndex();
	}

	// 3. 最終出力先へ：元画像コピー ＋ Bloom加算
	RebindFinalOutput(commandList);
	copy_.Execute(commandList, inputSrvIndex);              // 元シーン（不透明）
	composite_.Execute(commandList, blurRT2_->GetSrvIndex()); // Bloom（加算）
}

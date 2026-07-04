#include "RenderTarget.h"
#include "DirectXCommon.h"
#include <SrvManager.h>
#include <cassert>

void RenderTarget::Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height,
	DXGI_FORMAT format, const Vector4& clearColor)
{
	dxCommon_ = dxCommon;
	clearColor_ = clearColor;

	// カラーテクスチャを生成(生成直後の状態は PIXEL_SHADER_RESOURCE)
	resource_ = dxCommon_->CreateRenderTextureResource(
		dxCommon_->GetDevice(), width, height, format, clearColor);
	currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// この描画先専用のRTVヒープ(1個)を生成
	rtvHeap_ = dxCommon_->CreateDescriptorHeap(
		dxCommon_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);
	rtvHandle_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	dxCommon_->GetDevice()->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);

	// SRVを確保して生成
	srvIndex_ = SrvManager::GetInstance()->Allocate();
	SrvManager::GetInstance()->CreateSRVforTexture2D(srvIndex_, resource_.Get(), format, 1);
}

void RenderTarget::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList)
{
	if (currentState_ == D3D12_RESOURCE_STATE_RENDER_TARGET) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource_.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);
	currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void RenderTarget::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList)
{
	if (currentState_ == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource_.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	commandList->ResourceBarrier(1, &barrier);
	currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

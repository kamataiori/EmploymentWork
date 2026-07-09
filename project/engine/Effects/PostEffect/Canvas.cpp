#include "Canvas.h"
#include "DirectXCommon.h"

void Canvas::Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height,
	const Vector4& clearColor)
{
	dxCommon_ = dxCommon;
	width_ = width;
	height_ = height;
	clearColor_ = clearColor;

	sceneRT_ = std::make_unique<RenderTarget>();
	sceneRT_->Initialize(dxCommon_, width_, height_,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor_);

	// エフェクト適用後の結果を書き出す先
	outputRT_ = std::make_unique<RenderTarget>();
	outputRT_->Initialize(dxCommon_, width_, height_,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor_);

	// このレイヤー専用のポストエフェクトスタック（初期はNormal＝素通し）
	effect_ = std::make_unique<OffscreenRendering>();
	effect_->Initialize(PostEffectType::Normal);

	resultSrvIndex_ = sceneRT_->GetSrvIndex();
}

void Canvas::BeginScene(ID3D12GraphicsCommandList* commandList, bool clearDepth)
{
	sceneRT_->TransitionToRenderTarget(commandList);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneRT_->GetRtvHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();
	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

	// カラークリア
	float clear[4] = { clearColor_.x, clearColor_.y, clearColor_.z, clearColor_.w };
	commandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
	// 深度クリア（共有したい場合はスキップしてエンジン深度を維持）
	if (clearDepth) {
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	// ビューポート・シザー
	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(width_);
	viewport.Height = static_cast<float>(height_);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissor{};
	scissor.right = static_cast<LONG>(width_);
	scissor.bottom = static_cast<LONG>(height_);
	commandList->RSSetScissorRects(1, &scissor);
}

void Canvas::EndScene(ID3D12GraphicsCommandList* commandList)
{
	sceneRT_->TransitionToShaderResource(commandList);
}

void Canvas::Resolve(ID3D12GraphicsCommandList* commandList)
{
	// このレイヤーのエフェクトを sceneRT を入力に outputRT へ適用する。
	// エフェクトが Normal でも単純コピーとして outputRT へ転写される。
	effect_->ApplyTo(commandList, sceneRT_->GetSrvIndex(), outputRT_.get());
	outputRT_->TransitionToShaderResource(commandList);
	resultSrvIndex_ = outputRT_->GetSrvIndex();
}

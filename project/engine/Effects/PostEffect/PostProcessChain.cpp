#include "PostProcessChain.h"
#include "DirectXCommon.h"

void PostProcessChain::Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height)
{
	dxCommon_ = dxCommon;
	width_ = width;
	height_ = height;
	// 中間バッファはマルチパスが実際に必要になるまで生成しない（遅延生成）
}

void PostProcessChain::EnsurePingPong()
{
	if (pingPongReady_) {
		return;
	}
	const Vector4 kClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	for (auto& rt : pingPong_) {
		rt = std::make_unique<RenderTarget>();
		rt->Initialize(dxCommon_, width_, height_,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kClearColor);
	}
	pingPongReady_ = true;
}

void PostProcessChain::BindRenderTarget(ID3D12GraphicsCommandList* commandList, RenderTarget* target)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = target->GetRtvHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxCommon_->GetDsvHandle();
	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(width_);
	viewport.Height = static_cast<float>(height_);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissor{};
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = static_cast<LONG>(width_);
	scissor.bottom = static_cast<LONG>(height_);
	commandList->RSSetScissorRects(1, &scissor);
}

void PostProcessChain::Execute(ID3D12GraphicsCommandList* commandList, uint32_t sceneSrvIndex,
	RenderTarget* finalOutput)
{
	if (passes_.empty()) {
		// パスが無い場合は何もしない（呼び出し側が別途コピー／合成する想定）
		return;
	}

	// 最終出力先をバインドするラムダ（nullptr=バックバッファ）
	auto bindFinal = [&]() {
		if (finalOutput) {
			finalOutput->TransitionToRenderTarget(commandList);
			BindRenderTarget(commandList, finalOutput);
		} else {
			dxCommon_->BindSwapChainRenderTarget();
		}
	};

	if (passes_.size() == 1) {
		// 単一パス：最終出力先へ直接描画
		passes_[0]->SetFinalOutput(dxCommon_, finalOutput);
		bindFinal();
		passes_[0]->Execute(commandList, sceneSrvIndex);
		return;
	}

	// マルチパス：中間バッファを ping-pong しながら適用
	EnsurePingPong();

	uint32_t inputSrv = sceneSrvIndex;
	for (size_t i = 0; i < passes_.size(); ++i) {
		const bool isLast = (i + 1 == passes_.size());
		if (!isLast) {
			RenderTarget* target = pingPong_[i % 2].get();
			target->TransitionToRenderTarget(commandList);
			BindRenderTarget(commandList, target);
			passes_[i]->Execute(commandList, inputSrv);
			target->TransitionToShaderResource(commandList);
			inputSrv = target->GetSrvIndex();
		} else {
			// 最後のパスは最終出力先へ
			passes_[i]->SetFinalOutput(dxCommon_, finalOutput);
			bindFinal();
			passes_[i]->Execute(commandList, inputSrv);
		}
	}
}

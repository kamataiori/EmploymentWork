#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <Vector4.h>

class DirectXCommon;

/// <summary>
/// 1枚のオフスクリーン描画先(カラーテクスチャ + 専用RTV + SRV)をまとめた再利用クラス。
/// Canvasやポストエフェクトの中間バッファとして複数生成できる。
/// 生成直後の状態は PIXEL_SHADER_RESOURCE。
/// </summary>
class RenderTarget {
public:
	/// <summary>
	/// 初期化。カラーテクスチャ・専用RTVヒープ(1個)・SRV(SrvManager)を生成する。
	/// </summary>
	void Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height,
		DXGI_FORMAT format, const Vector4& clearColor);

	/// <summary>
	/// この描画先を RENDER_TARGET 状態へ遷移させる(描画先として使う前に呼ぶ)。
	/// </summary>
	void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);

	/// <summary>
	/// この描画先を PIXEL_SHADER_RESOURCE 状態へ遷移させる(SRVとして読む前に呼ぶ)。
	/// </summary>
	void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

	/// <summary>RTVのCPUハンドルを取得</summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return rtvHandle_; }

	/// <summary>SrvManager上のSRVインデックスを取得</summary>
	uint32_t GetSrvIndex() const { return srvIndex_; }

	/// <summary>クリアカラーを取得</summary>
	const Vector4& GetClearColor() const { return clearColor_; }

	/// <summary>カラーテクスチャリソースを取得</summary>
	ID3D12Resource* GetResource() const { return resource_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;

	// カラーテクスチャ本体
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	// この描画先専用のRTVヒープ(1個)
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};

	// SrvManagerから確保したSRVインデックス
	uint32_t srvIndex_ = 0;

	Vector4 clearColor_{};

	// 現在のリソース状態(バリアの自動判定に使う)
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};

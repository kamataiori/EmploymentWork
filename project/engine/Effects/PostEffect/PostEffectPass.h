#pragma once
#include "IPostEffectPass.h"
#include "DirectXCommon.h"
#include <Vector2.h>
#include <Vector3.h>
#include <string>
#include <wrl.h>
#include <cassert>
#include <externals/DirectXTex/d3dx12.h>

/// <summary>
/// ポストエフェクトパスの共通基底。
/// 「SRV(t0) を1枚入力し、フルスクリーン三角形で書き出す」という
/// 全パス共通の RootSignature / PSO / 描画処理をまとめる。
/// 定数バッファを持つエフェクトは UseConstantBuffer() を true にし、
/// Execute() をオーバーライドして CBV(b0) をバインドする。
/// </summary>
class PostEffectPassBase : public IPostEffectPass {
public:
	void Initialize(DirectXCommon* dxCommon) override;

	// 既定の Execute：SRVをバインドしてフルスクリーン描画（CopyPass=Normal 用）
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	// 派生が指定するピクセルシェーダのパス
	virtual std::wstring GetPixelShaderPath() const = 0;
	// 定数バッファ(b0)を使うか
	virtual bool UseConstantBuffer() const { return false; }
	// PSOのブレンド設定（既定は不透明の上書き。合成/加算パスでオーバーライドする）
	virtual D3D12_BLEND_DESC GetBlendDesc() const { return CD3DX12_BLEND_DESC(D3D12_DEFAULT); }
	// Initialize 完了後フック（定数バッファ生成などに使う）
	virtual void OnInitialized() {}

	// SRV(t0) [+ CBV(b0)] の標準ルートシグネチャを生成
	void CreateStandardRootSignature(bool useCbv);
	// フルスクリーン三角形PSOを生成（VS共通、PSはpsPath）
	void CreatePipeline(const std::wstring& psPath);
	// PSO・RootSignature・入力SRVテーブル(param0)をバインド
	void BindCommon(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex);
	// フルスクリーン三角形をDraw
	void DrawFullscreen(ID3D12GraphicsCommandList* commandList);

	// 定数バッファ生成ヘルパ（UPLOADヒープ、256アライン、Map済み）
	template<class T>
	void CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& resource, T*& mapped);

protected:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};

template<class T>
inline void PostEffectPassBase::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& resource, T*& mapped)
{
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(T) + 0xFF) & ~0xFF);
	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	resource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

//========================================================================
// CopyPass（Normal）：入力をそのまま出力に不透明コピー
//========================================================================
class CopyPass : public PostEffectPassBase {
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/CopyImage.PS.hlsl"; }
};

//========================================================================
// AlphaBlendCopyPass：入力をアルファブレンドで重ねる（レイヤー合成用）
//========================================================================
class AlphaBlendCopyPass : public PostEffectPassBase {
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/CopyImage.PS.hlsl"; }
	D3D12_BLEND_DESC GetBlendDesc() const override {
		D3D12_BLEND_DESC desc{};
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		return desc;
	}
};

//========================================================================
// AdditiveBlendCopyPass：入力を加算合成で重ねる（レイヤー合成用）
// パーティクルはアルファを書かず「色×アルファ」がレイヤーに入るため、
// 加算（backbuffer += layer.rgb）で重ねるのが正しい。
//========================================================================
class AdditiveBlendCopyPass : public PostEffectPassBase {
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/CopyImage.PS.hlsl"; }
	D3D12_BLEND_DESC GetBlendDesc() const override {
		D3D12_BLEND_DESC desc{};
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		return desc;
	}
};

//========================================================================
// VignettePass
//========================================================================
class VignettePass : public PostEffectPassBase {
public:
	void SetParams(float scale, float power, const Vector3& color);
	void SetScale(float scale) { if (mapped_) mapped_->scale = scale; }
	void SetPower(float power) { if (mapped_) mapped_->power = power; }
	void SetColor(const Vector3& color) { if (mapped_) mapped_->color = color; }

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/Vignette.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }

private:
	struct VignetteCB {
		float scale;
		float power;
		float padding[2];
		Vector3 color;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	VignetteCB* mapped_ = nullptr;
};

//========================================================================
// GrayscalePass
//========================================================================
class GrayscalePass : public PostEffectPassBase {
public:
	void SetParams(float strength, const Vector3& weights);
	void SetStrength(float strength) { if (mapped_) mapped_->strength = strength; }
	void SetWeights(const Vector3& weights) { if (mapped_) mapped_->weights = weights; }

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/Grayscale.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }

private:
	struct GrayscaleCB {
		float strength;
		Vector3 weights;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	GrayscaleCB* mapped_ = nullptr;
};

//========================================================================
// SepiaPass
//========================================================================
class SepiaPass : public PostEffectPassBase {
public:
	void SetParams(const Vector3& color, float strength);
	void SetColor(const Vector3& color) { if (mapped_) mapped_->color = color; }
	void SetStrength(float strength) { if (mapped_) mapped_->strength = strength; }

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/Sepia.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }

private:
	struct SepiaCB {
		Vector3 color;
		float strength;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	SepiaCB* mapped_ = nullptr;
};

//========================================================================
// RadialBlurPass
//========================================================================
class RadialBlurPass : public PostEffectPassBase {
public:
	void SetParams(const Vector2& center, float blurWidth, int numSamples);
	void SetCenter(const Vector2& center) { if (mapped_) mapped_->center = center; }
	void SetWidth(float blurWidth) { if (mapped_) mapped_->blurWidth = blurWidth; }
	void SetSamples(int numSamples) { if (mapped_) mapped_->numSamples = numSamples; }

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/RadialBlur.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }

private:
	struct RadialBlurCB {
		Vector2 center;
		float blurWidth;
		int numSamples;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	RadialBlurCB* mapped_ = nullptr;
};

//========================================================================
// RandomPass
//========================================================================
class RandomPass : public PostEffectPassBase {
public:
	void SetParams(float time, float scale, float intensity);
	void SetTime(float time) { if (mapped_) mapped_->time = time; }
	void SetScale(float scale) { if (mapped_) mapped_->scale = scale; }
	void SetIntensity(float intensity) { if (mapped_) mapped_->intensity = intensity; }
	void SetUseImage(bool useImage) { if (mapped_) mapped_->useImage = useImage ? 1.0f : 0.0f; }

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/Random.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }

private:
	struct RandomCB {
		float time;
		float scale;
		float intensity;
		float useImage;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	RandomCB* mapped_ = nullptr;
};

//========================================================================
// DissolvePass（特殊：SRVを2枚 t0/t1 使い、入力SRVは使わない）
//========================================================================
class DissolvePass : public PostEffectPassBase {
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void SetParams(float threshold, float edgeWidth, const Vector3& edgeColor);
	void SetThreshold(float value) { if (mapped_) mapped_->threshold = value; }
	void SetEdgeWidth(float value) { if (mapped_) mapped_->edgeWidth = value; }
	void SetEdgeColor(const Vector3& color) { if (mapped_) mapped_->edgeColor = color; }
	void SetTextures(const std::string& scenePath, const std::string& noisePath);

	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;

protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/Dissolve.PS.hlsl"; }

private:
	// SRV2つ(t0/t1) + CBV(b0) の専用ルートシグネチャ
	void CreateDissolveRootSignature();

	struct DissolveCB {
		float threshold;
		float edgeWidth;
		Vector3 edgeColor;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	DissolveCB* mapped_ = nullptr;

	std::string scenePath_;
	std::string noisePath_;
};

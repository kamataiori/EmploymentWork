#pragma once
#include "PostEffectPass.h"
#include "RenderTarget.h"
#include <memory>
#include <Vector2.h>

//========================================================================
// Bloom内部で使う小さなパス群
//========================================================================

// 明るい部分を抽出するパス（しきい値CB）
class BloomExtractPass : public PostEffectPassBase {
public:
	void SetThreshold(float threshold) { if (mapped_) mapped_->threshold = threshold; }
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/BloomExtract.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }
	void OnInitialized() override;
private:
	struct ExtractCB { float threshold; float padding[3]; };
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	ExtractCB* mapped_ = nullptr;
};

// 方向性ガウシアンブラーのパス（方向CB）
class BloomBlurPass : public PostEffectPassBase {
public:
	void SetDirection(const Vector2& direction) { if (mapped_) mapped_->direction = direction; }
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/BloomBlur.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }
	void OnInitialized() override;
private:
	struct BlurCB { Vector2 direction; float padding[2]; };
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	BlurCB* mapped_ = nullptr;
};

// ぼかしたBloomを intensity 倍して加算合成するパス
class BloomCompositePass : public PostEffectPassBase {
public:
	void SetIntensity(float intensity) { if (mapped_) mapped_->intensity = intensity; }
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;
protected:
	std::wstring GetPixelShaderPath() const override { return L"Resources/shaders/BloomComposite.PS.hlsl"; }
	bool UseConstantBuffer() const override { return true; }
	// 元画像の上に加算合成する
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
	void OnInitialized() override;
private:
	struct CompositeCB { float intensity; float padding[3]; };
	Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
	CompositeCB* mapped_ = nullptr;
};

//========================================================================
// BloomPass：明るい部分抽出 → ガウスぼかし(H/V) → 元画像に加算合成
// 内部で複数RTを回す自己完結型のマルチパス効果。
//========================================================================
class BloomPass : public IPostEffectPass {
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) override;
	void SetFinalOutput(DirectXCommon* dxCommon, RenderTarget* output) override;

	// パラメータ
	void SetThreshold(float threshold) { extract_.SetThreshold(threshold); }
	void SetIntensity(float intensity) { composite_.SetIntensity(intensity); }
	// ぼかしの反復回数（多いほど広く強いにじみ）
	void SetIterations(int iterations) { blurIterations_ = iterations < 1 ? 1 : iterations; }

private:
	// 内部RTをRTVとしてバインド（半解像度・DSVなし）
	void BindInternalRT(ID3D12GraphicsCommandList* commandList, RenderTarget* target);
	// 最終出力先を再バインド（内部RT描画のあとに戻す）
	void RebindFinalOutput(ID3D12GraphicsCommandList* commandList);

private:
	DirectXCommon* dxCommon_ = nullptr;

	// 内部パス
	BloomExtractPass extract_;
	BloomBlurPass blurH_;
	BloomBlurPass blurV_;
	BloomCompositePass composite_;
	CopyPass copy_;   // 元画像をそのまま出力へコピー

	// 内部RT（半解像度）
	std::unique_ptr<RenderTarget> extractRT_;
	std::unique_ptr<RenderTarget> blurRT1_;
	std::unique_ptr<RenderTarget> blurRT2_;

	uint32_t bloomWidth_ = 0;
	uint32_t bloomHeight_ = 0;

	// ぼかしの反復回数
	int blurIterations_ = 3;

	// 最終出力先（nullptr=バックバッファ）
	RenderTarget* finalOutput_ = nullptr;
};

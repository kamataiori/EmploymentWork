#pragma once
#include "DirectXCommon.h"
#include <array>
#include <Vector3.h>
#include <Vector2.h>

//class DirectXCommon;

enum class PostEffectType {
	Normal,
	Blur5x5,
	Blur3x3,
	GaussianFilter,
	RadialBlur,
	Grayscale,
	Vignette,
	Sepia,
	Random,
	Dissolve,
	// 追加可能
	Count
};

constexpr size_t kPostEffectCount = static_cast<size_t>(PostEffectType::Count);

class OffscreenRendering
{
public:  // publicメンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(PostEffectType type);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// 
	uint32_t GetSrvIndex() const { return srvIndex_; }

	/// <summary>
	/// ポストエフェクトの種類を設定
	/// </summary>
	void SetPostEffectType(PostEffectType type);

	/// <summary>
	/// 現在のポストエフェクトの種類を取得
	/// </summary>
	PostEffectType GetPostEffectType() const;

	/// <summary>
	/// ビネット専用の定数バッファ初期化
	/// </summary>
	void VignetteInitialize(float scale, float power, const Vector3& color);

	/// <summary>
	/// ビネット：スケールの変更
	/// </summary>
	void SetVignetteScale(float scale);

	/// <summary>
	/// ビネット：パワーの変更
	/// </summary>
	void SetVignettePower(float power);

	/// <summary>
	/// ビネット：カラーの変更
	/// </summary>
	void SetVignetteColor(const Vector3& color);

	/// <summary>
	/// グレースケール専用の定数バッファ初期化
	/// </summary>
	void GrayscaleInitialize(float strength, const Vector3& weights);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="strength"></param>
	void SetGrayscaleStrength(float strength);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="weights"></param>
	void SetGrayscaleWeights(const Vector3& weights);

	/// <summary>
	/// セピア専用の定数バッファ初期化
	/// </summary>
	void SepiaInitialize(const Vector3& sepiaColor, float strength);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="sepiaColor"></param>
	void SetSepiaColor(const Vector3& sepiaColor);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="strength"></param>
	void SetSepiaStrength(float strength);

	/// <summary>
	/// ラジアルブラー専用の定数バッファ初期化
	/// </summary>
	void RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples);

	/// <summary>
	/// 
	/// </summary>
	void SetRadialBlurCenter(const Vector2& center);

	/// <summary>
	/// 
	/// </summary>
	void SetRadialBlurWidth(float blurWidth);

	/// <summary>
	/// 
	/// </summary>
	void SetRadialBlurSamples(int numSamples);

	/// <summary>
	/// ランダム専用の定数バッファ初期化
	/// </summary>
	void RandomInitialize(float time, float scale, float intensity);

	/// <summary>
	/// 
	/// </summary>
	void SetRandomTime(float time);
	
	/// <summary>
	/// 
	/// </summary>
	void SetRandomScale(float scale);

	/// <summary>
	/// 
	/// </summary>
	void SetRandomIntensity(float intensity);

	/// <summary>
	/// 
	/// </summary>
	void SetRandomUseImage(bool useImage);

	/// <summary>
	/// ディゾルブ専用の定数バッファ初期化
	/// </summary>
	void DissolveInitialize(float threshold, float edgeWidth, const Vector3& edgeColor);
	void SetDissolveThreshold(float value);
	void SetDissolveEdgeWidth(float value);
	void SetDissolveEdgeColor(const Vector3& color);
	void SetDissolveTextures(const std::string& scenePath, const std::string& noisePath);

private:  // privateメンバ関数

	/// <summary>
	/// RTVの生成
	/// </summary>
	void RenderTexture();

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void RootSignature();

	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	//void GraphicsPipelineState(PostEffectType type);

	/// <summary>
	/// InputLayoutの設定
	/// </summary>
	void InputLayout();

	/// <summary>
	/// BlendStateの設定
	/// </summary>
	void BlendState();

	/// <summary>
	/// RasterizerStateの設定
	/// </summary>
	void RasterizerState();

	/// <summary>
	/// DepthStencilStateの設定
	/// </summary>
	void DepthStencilState();

	/// <summary>
	/// PSOの生成
	/// </summary>
	void PSO();

	/// <summary>
	/// 全てのポストエフェクト用PSOを生成
	/// </summary>
	void CreateAllPSOs();

	/// <summary>
	/// 全てのRootSignatureを生成
	/// </summary>
	void CreateAllRootSignatures();


public:  // publicメンバ変数



private:  // privateメンバ変数

	// DirectXCommonの初期化
	DirectXCommon* dxCommon_;

	// シェーダーリソースビューのインデックス
	uint32_t srvIndex_ = 0;

	//--------RootSignature部分--------//

	// DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange_[1] = {};

	// RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
	D3D12_ROOT_PARAMETER rootParameters_[3] = {};

	// バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;


	//--------InputLayout部分--------//

	// InputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{};
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};


	//--------BlendState部分--------//

	// BlendState
	D3D12_BLEND_DESC blendDesc_{};


	//--------RasterizerState部分--------//

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc_{};


	//--------DepthStencilState部分--------//

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};


	//--------PSO部分--------//

	// PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	// graphicsPipelineStateの生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_{};

	// PSO配列（各PostEffectTypeごと）
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kPostEffectCount> pipelineStates_{};
	// RootSignature配列（各PostEffectTypeごと）
	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, kPostEffectCount> rootSignatures_{};

	// 現在のエフェクトタイプ
	PostEffectType currentType_ = PostEffectType::Normal;


	//--------Vignette定数バッファ--------//

	// Vignette用定数バッファ構造体
	struct VignetteCB {
		float vignetteScale;
		float vignettePower;
		float padding[2];
		Vector3 vignetteColor;
	};

	// リソースとマッピングポインタ
	Microsoft::WRL::ComPtr<ID3D12Resource> vignetteCB_;
	VignetteCB* mappedVignetteCB_ = nullptr;

	//--------Grayscale定数バッファ--------//
	struct GrayscaleCB {
		float strength;
		Vector3 weights;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> grayscaleCB_;
	GrayscaleCB* mappedGrayscaleCB_ = nullptr;

	//--------Sepia定数バッファ--------//
	struct SepiaCB {
		Vector3 sepiaColor;
		float sepiaStrength;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> sepiaCB_;
	SepiaCB* mappedSepiaCB_ = nullptr;

	//--------RadialBlur定数バッファ--------//

	struct RadialBlurCB {
		Vector2 center;
		float blurWidth;
		int numSamples;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> radialBlurCB_;
	RadialBlurCB* mappedRadialBlurCB_ = nullptr;

	//--------Random定数バッファ--------//

	struct RandomCB { 
		float time; 
		float scale; 
		float intensity; 
		float useImage;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> randomCB_;
	RandomCB* mappedRandomCB_ = nullptr;

	//--------Dissolve定数バッファ--------//

	// Dissolve定数バッファ
	struct DissolveCB {
		float threshold;
		float edgeWidth;
		Vector3 edgeColor;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> dissolveCB_;
	DissolveCB* mappedDissolveCB_ = nullptr;

	// Dissolve用テクスチャパス
	std::string dissolveScenePath_;
	std::string dissolveNoisePath_;


};
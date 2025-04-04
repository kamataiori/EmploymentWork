#pragma once
#include <DirectXCommon.h>
#include "StructAnimation.h"

class Skinning
{
public:
	static Skinning* instance;

	// インスタンスを取得するシングルトンメソッド
	static Skinning* GetInstance();

	// プライベートコンストラクタ
	Skinning() = default;

	// コピーコンストラクタおよび代入演算子を削除
	Skinning(const Skinning&) = delete;
	Skinning& operator=(const Skinning&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

private:

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void RootSignature();

	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	void GraphicsPipelineState();

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

public:

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void CommonSetting();

private:

	//DirectXCommonの初期化
	DirectXCommon* dxCommon_;

	D3D12_DESCRIPTOR_RANGE DescriptorRange_[1] = {};

	D3D12_ROOT_SIGNATURE_DESC DescriptionRootSignature_{};

	D3D12_ROOT_PARAMETER RootParameters_[8] = {};

	std::array<D3D12_INPUT_ELEMENT_DESC, 5> InputElementDescs_{};

	D3D12_BLEND_DESC BlendDesc_{};

	D3D12_RASTERIZER_DESC RasterizerDesc_{};

	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc_{};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc_{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature_ = nullptr;

	D3D12_INPUT_LAYOUT_DESC InputLayoutDesc_{};

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> VertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> PixelShaderBlob_{};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GraphicsPipelineState_ = nullptr;
};


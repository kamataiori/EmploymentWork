#pragma once
#include "DirectXCommon.h"

class DrawTriangleCommon
{
public:  // publicメンバ関数

	// インスタンスを取得
	static DrawTriangleCommon* GetInstance();

	void Finalize();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void CommonSetting();

private:  // privateメンバ関数

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

public:  // publicメンバ変数

	struct Vertex {
		float position[3]; // x, y, z 座標
		float color[4];    // r, g, b, a 色
	};


private:  // privateメンバ変数

	// コンストラクタとデストラクタをプライベートに
	DrawTriangleCommon() = default;
	~DrawTriangleCommon() = default;

	// コピーコンストラクタと代入演算子を削除
	DrawTriangleCommon(const DrawTriangleCommon&) = delete;
	DrawTriangleCommon& operator=(const DrawTriangleCommon&) = delete;

	static DrawTriangleCommon* instance;

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
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};


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
};


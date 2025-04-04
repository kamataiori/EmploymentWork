#pragma once
#include "DirectXCommon.h"
#include "Logger.h"

class SpriteCommon
{
public:

	static SpriteCommon* instance;

	// インスタンスを取得するシングルトンメソッド
	static SpriteCommon* GetInstance();

	// プライベートコンストラクタ
	SpriteCommon() = default;

	// コピーコンストラクタおよび代入演算子を削除
	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;


public:

	///===========================
	// メンバ関数
	///===========================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

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

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void CommonSetting();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();


	////======ゲッター関数======////

	DirectXCommon* GetDxCommon() const { return dxCommon_; }




private:

	DirectXCommon* dxCommon_;

	//DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange_[1] = {};

	//RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
	D3D12_ROOT_PARAMETER rootParameters_[3] = {};

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	//InputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_;
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

	//BlendState
	D3D12_BLEND_DESC blendDesc_{};

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc_{};

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_{};

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};

	//graphicsPipelineStateの生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;

	//PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

};


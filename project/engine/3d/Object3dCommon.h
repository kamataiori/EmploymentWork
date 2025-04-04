#pragma once
#include "DirectXCommon.h"
#include "Camera.h"

//BlendMode
enum BlendMode {
	//!< ブレンドなし
	kBlendModeNone,
	//!< 通常αブレンド。デフォルト。Src * SrcA + Dest * (1 - SrcA)
	kBlendModeNormal,
	//!< 加算。Src * SrcA + Dest * 1
	kBlendModeAdd,
	//!< 減算。Dest * 1 - Src * SrcA
	kBlendModeSubtract,
	//!< 乗算。Src * 0 + Dest * Src
	kBlendModeMultiply,
	//!< スクリーン。Src * (1 - Dest) + Dest * 1
	kBlendModeScreen,
	// 利用してはいけない
	kCountOfBlendMode,
};

class Object3dCommon
{
public:
	static Object3dCommon* instance;

	// インスタンスを取得するシングルトンメソッド
	static Object3dCommon* GetInstance();

	// プライベートコンストラクタ
	Object3dCommon() = default;

	// コピーコンストラクタおよび代入演算子を削除
	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// デフォルトのカメラを取得
	/// </summary>
	Camera* GetDefaultCamera();

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

public:
	//------Getter------//
	//DirectXCommongetter
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	Camera* GetDefaultCamera() const { return defaultCamera; }

	//------Setter------//
	void SetDefaultCamera(Camera* camera) { this->defaultCamera = camera; }


private:
	//DirectXCommonの初期化
	DirectXCommon* dxCommon_;

	//--------RootSignature部分--------//

	//DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange_[1] = {};

	//RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
	D3D12_ROOT_PARAMETER rootParameters_[7] = {};

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;


	//--------InputLayout部分--------//

	//InputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{};
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};


	//--------BlendState部分--------//

	//BlendState
	D3D12_BLEND_DESC blendDesc_{};


	//--------RasterizerState部分--------//

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc_{};


	//--------DepthStencilState部分--------//

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};


	//--------PSO部分--------//

	//PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//graphicsPipelineStateの生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_{};

	//デフォルトカメラ
	Camera* defaultCamera = nullptr;
};


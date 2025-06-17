#pragma once
#include <DirectXCommon.h>
#include "StructAnimation.h"
#include "Model.h"

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
	/// CS用ルートシグネチャの作成
	/// </summary>
	void RootSignatureCS();

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

private:

	/// <summary>
	/// ComputeShaderのPSO
	/// </summary>
	void ComputePipelineState();

public:

	/// <summary>
    /// Compute用共通設定
    /// </summary>
	void CommonSettingCompute(SkinCluster skinCluster);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void CommonSetting();

	ID3D12PipelineState* GetComputePipelineState() const { return ComputePipelineState_.Get(); }
	ID3D12RootSignature* GetComputeRootSignature() const { return ComputeRootSignature_.Get(); }

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

	//------ComputeShader------//

	// ShaderBlob
	Microsoft::WRL::ComPtr<IDxcBlob> ComputeShaderBlob_ = nullptr;

	// RootSignature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputeRootSignature_ = nullptr;
	D3D12_ROOT_SIGNATURE_DESC ComputeDescriptionRootSignature_{};
	D3D12_ROOT_PARAMETER ComputeRootParameters_[5] = {};

	// Compute用PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipelineState_ = nullptr;

	// UAV用リソースとディスクリプタ
	Microsoft::WRL::ComPtr<ID3D12Resource> outputBufferResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputUavHandle_;


	// SRVリソースのハンドル（t0, t1, t2）
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> palette_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> inputVertex_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influence_;

	// UAVリソースのハンドル（u0）
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputVertex_;

	// CBVリソース（b0）
	struct SkinningInformation {
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	};
	SkinningInformation skinningInformation_;

};


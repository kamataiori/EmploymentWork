#include "Skinning.h"
#include <Logger.h>
#include <externals/DirectXTex/d3dx12.h>

Skinning* Skinning::instance = nullptr;

Skinning* Skinning::GetInstance()
{
	if (!instance) {
		instance = new Skinning();
	}
	return instance;
}

void Skinning::Initialize(bool isAnimation)
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();

	// Compute用のPSO
	ComputePipelineState();

	// グラフィックスパイプラインの生成
	GraphicsPipelineState();

}

void Skinning::Finalize()
{
	delete instance;
	instance = nullptr;
}

void Skinning::RootSignature()
{
	////=========DescriptorRange=========////

	DescriptorRange_[0].BaseShaderRegister = 0;  //0から始まる
	DescriptorRange_[0].NumDescriptors = 1;  //数は1
	DescriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  //SRVを使う
	DescriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  //Offsetを自動計算


	////=========RootSignatureを生成する=========////

	//RootSignature作成
	DescriptionRootSignature_.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//RootSignature作成
	RootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
	RootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;    //PixelShaderで使う
	RootParameters_[0].Descriptor.ShaderRegister = 0;    //レジスタ番号0とバインド

	RootParameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
	RootParameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;    //VertexShaderで使う
	RootParameters_[1].Descriptor.ShaderRegister = 0;    //レジスタ番号0を使う

	/*rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);*/

	RootParameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  //DescriptorTableを使う
	RootParameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	RootParameters_[2].DescriptorTable.pDescriptorRanges = DescriptorRange_;
	RootParameters_[2].DescriptorTable.NumDescriptorRanges = _countof(DescriptorRange_);
	////=========平行光源をShaderで使う=========////
	RootParameters_[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	RootParameters_[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	RootParameters_[3].Descriptor.ShaderRegister = 1;  //レジスタ番号1を使う

	////=========光源のカメラの位置をShaderで使う=========////
	RootParameters_[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	RootParameters_[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	RootParameters_[4].Descriptor.ShaderRegister = 2;  //レジスタ番号2を使う

	////========ポイントライトをShaderで使う========////
	RootParameters_[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	RootParameters_[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	RootParameters_[5].Descriptor.ShaderRegister = 3;  //レジスタ番号3を使う

	////========スポットライトをShaderで使う========////
	RootParameters_[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	RootParameters_[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	RootParameters_[6].Descriptor.ShaderRegister = 4;  //レジスタ番号4を使う

	////========StructuredBufferをShaderで使う========////
	RootParameters_[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  //DescriptorTableを使う
	RootParameters_[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	RootParameters_[7].DescriptorTable.pDescriptorRanges = DescriptorRange_;
	RootParameters_[7].DescriptorTable.NumDescriptorRanges = _countof(DescriptorRange_);

	DescriptionRootSignature_.pParameters = RootParameters_;    //ルートパラメータ配列へのポインタ
	DescriptionRootSignature_.NumParameters = _countof(RootParameters_);    //配列の長さ

	////=========Samplerの設定=========////

	D3D12_STATIC_SAMPLER_DESC StaticSamplers[1] = {};
	StaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  //バイリニアフィルタ
	StaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //0～1の範囲外をリピート
	StaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  //比較しない
	StaticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;  //ありったけのMipmapを使う
	StaticSamplers[0].ShaderRegister = 0;  //レジスタ番号0を使う
	StaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	DescriptionRootSignature_.pStaticSamplers = StaticSamplers;
	DescriptionRootSignature_.NumStaticSamplers = _countof(StaticSamplers);


	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> SignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&DescriptionRootSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(ErrorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	hr = dxCommon_->GetDevice().Get()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(),
		SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&RootSignature_));
	assert(SUCCEEDED(hr));
}

void Skinning::RootSignatureCS()
{
	D3D12_DESCRIPTOR_RANGE DescriptorRangeUAV[4] = {};
	DescriptorRangeUAV[0].BaseShaderRegister = 0;  // register(t0)
	DescriptorRangeUAV[0].NumDescriptors = 1;
	DescriptorRangeUAV[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ←重要！
	DescriptorRangeUAV[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	DescriptorRangeUAV[1].BaseShaderRegister = 1;  // register(t1)
	DescriptorRangeUAV[1].NumDescriptors = 1;
	DescriptorRangeUAV[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ←重要！
	DescriptorRangeUAV[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	DescriptorRangeUAV[2].BaseShaderRegister = 2;  // register(t2)
	DescriptorRangeUAV[2].NumDescriptors = 1;
	DescriptorRangeUAV[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ←重要！
	DescriptorRangeUAV[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	DescriptorRangeUAV[3].BaseShaderRegister = 0;  // register(u0)
	DescriptorRangeUAV[3].NumDescriptors = 1;
	DescriptorRangeUAV[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; // ←重要！
	DescriptorRangeUAV[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	////=========RootSignatureを生成する=========////

	//RootSignature作成
	ComputeDescriptionRootSignature_.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//RootSignature作成
	// 0.SkinningInfomation
	ComputeRootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
	ComputeRootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    //ComputeShaderで使う
	ComputeRootParameters_[0].Descriptor.ShaderRegister = 0;    //レジスタ番号0とバインド
	// 1.Palette
	ComputeRootParameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    //ComputeShaderで使う
	ComputeRootParameters_[1].DescriptorTable.pDescriptorRanges = &DescriptorRangeUAV[0];
	ComputeRootParameters_[1].DescriptorTable.NumDescriptorRanges = 1;
	// 2.InputVertices
	ComputeRootParameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    //ComputeShaderで使う
	ComputeRootParameters_[2].DescriptorTable.pDescriptorRanges = &DescriptorRangeUAV[1];
	ComputeRootParameters_[2].DescriptorTable.NumDescriptorRanges = 1;
	// 3.Influences
	ComputeRootParameters_[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters_[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    //ComputeShaderで使う
	ComputeRootParameters_[3].DescriptorTable.pDescriptorRanges = &DescriptorRangeUAV[2];
	ComputeRootParameters_[3].DescriptorTable.NumDescriptorRanges = 1;
	// 4.OutputVertices
	ComputeRootParameters_[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters_[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    //ComputeShaderで使う
	ComputeRootParameters_[4].DescriptorTable.pDescriptorRanges = &DescriptorRangeUAV[3];
	ComputeRootParameters_[4].DescriptorTable.NumDescriptorRanges = 1;

	ComputeDescriptionRootSignature_.pParameters = ComputeRootParameters_;    //ルートパラメータ配列へのポインタ
	ComputeDescriptionRootSignature_.NumParameters = _countof(ComputeRootParameters_);    //配列の長さ

	// シリアライズしてバイナリ化
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&ComputeDescriptionRootSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob);

	if (FAILED(hr)) {
		if (errorBlob) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}

	// ルートシグネチャを作成
	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&ComputeRootSignature_));
	assert(SUCCEEDED(hr));

}

void Skinning::GraphicsPipelineState()
{
	//ルートシグネチャの生成
	RootSignature();

	////=========InputLayoutの設定を行う=========////

	InputLayout();


	////=========BlendStateの設定を行う=========////

	BlendState();

	////=========RasterizerStateの設定を行う=========////

	RasterizerState();


	////=========ShaderをCompileする=========////

	VertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/SkinningObject3d.VS.hlsl", L"vs_6_0");
	if (VertexShaderBlob_ == nullptr) {
		Logger::Log("vertexShaderBlob_\n");
		exit(1);
	}
	assert(VertexShaderBlob_ != nullptr);
	PixelShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
	if (PixelShaderBlob_ == nullptr) {
		Logger::Log("pixelShaderBlob_\n");
		exit(1);
	}
	assert(PixelShaderBlob_ != nullptr);


	////=========DepthStencilStateの設定を行う=========////

	DepthStencilState();


	////=========PSOを生成する=========////

	PSO();
}

void Skinning::InputLayout()
{
	/*InputElementDescs_[0].SemanticName = "POSITION";
	InputElementDescs_[0].SemanticIndex = 0;
	InputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	InputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	InputElementDescs_[1].SemanticName = "TEXCOORD";
	InputElementDescs_[1].SemanticIndex = 0;
	InputElementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	InputElementDescs_[2].SemanticName = "NORMAL";
	InputElementDescs_[2].SemanticIndex = 0;
	InputElementDescs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	InputElementDescs_[3].SemanticName = "WEIGHT";
	InputElementDescs_[3].SemanticIndex = 0;
	InputElementDescs_[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	InputElementDescs_[3].InputSlot = 1;
	InputElementDescs_[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	InputElementDescs_[4].SemanticName = "INDEX";
	InputElementDescs_[4].SemanticIndex = 0;
	InputElementDescs_[4].Format = DXGI_FORMAT_R32G32B32A32_SINT;
	InputElementDescs_[4].InputSlot = 1;
	InputElementDescs_[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputLayoutDesc_.pInputElementDescs = InputElementDescs_.data();
	InputLayoutDesc_.NumElements = InputElementDescs_.size();*/

	// 共通3つ
	InputElementDescs_[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	InputElementDescs_[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	InputElementDescs_[2] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	InputLayoutDesc_.pInputElementDescs = InputElementDescs_.data();
	InputLayoutDesc_.NumElements = 3;
}

void Skinning::BlendState()
{
	//すべての色要素を書き込む
	BlendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//--------ノーマルブレンド--------//
	BlendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BlendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BlendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	//--------加算合成--------//
	/*blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/
	//--------減算合成--------//
	/*blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/
	//--------乗算合成--------//
	/*blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;*/
	//--------スクリーン合成--------//
	/*blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/

	//α値のブレンド設定で基本的には使わない
	BlendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	BlendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	BlendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
}

void Skinning::RasterizerState()
{
	////=========RasterizerStateの設定を行う=========////

	//RasterizerStateの設定
	//裏面(時計回り)を表示しない
	RasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	//三角形の中を塗りつぶす
	RasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;
	//カリングしない(裏面も表示させる)
	//RasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
}

void Skinning::DepthStencilState()
{
	////=========DepthStencilStateの設定を行う=========////

	//DepthStencilStateの設定
	//Depthの機能を有効化する
	DepthStencilDesc_.DepthEnable = true;
	//書き込みします
	DepthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//比較関数はLessEqual。つまり、近ければ描画される
	DepthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

void Skinning::PSO()
{
	////=========PSOを生成する=========////

	GraphicsPipelineStateDesc_.pRootSignature = RootSignature_.Get();    //RootSignature
	GraphicsPipelineStateDesc_.InputLayout = InputLayoutDesc_;    //InputLayout
	GraphicsPipelineStateDesc_.VS = { VertexShaderBlob_->GetBufferPointer(),
	VertexShaderBlob_->GetBufferSize() };    //VertexShader
	GraphicsPipelineStateDesc_.PS = { PixelShaderBlob_->GetBufferPointer(),
	PixelShaderBlob_->GetBufferSize() };    //PixelShader
	GraphicsPipelineStateDesc_.BlendState = BlendDesc_;    //BlendState
	GraphicsPipelineStateDesc_.RasterizerState = RasterizerDesc_;    //RasterizerState
	//書き込むRTVの情報
	GraphicsPipelineStateDesc_.NumRenderTargets = 1;
	GraphicsPipelineStateDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジ(形状)のタイプ。三角形
	GraphicsPipelineStateDesc_.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むかの設定(気にしなくて良い)
	GraphicsPipelineStateDesc_.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//DepthStencilの設定
	GraphicsPipelineStateDesc_.DepthStencilState = DepthStencilDesc_;
	GraphicsPipelineStateDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//実際に生成
	HRESULT hr = dxCommon_->GetDevice().Get()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc_,
		IID_PPV_ARGS(&GraphicsPipelineState_));
	assert(SUCCEEDED(hr));
}

void Skinning::ComputePipelineState()
{
	//ルートシグネチャの生成
	RootSignatureCS();

	// ComputeShader をコンパイル
	ComputeShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Skinning.CS.hlsli", L"cs_6_0");
	assert(ComputeShaderBlob_ != nullptr);

	// RootSignature を作っておく（すでに RootSignature_ にあるなら流用OK）

	// ComputePipeline の作成
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{};
	computePipelineStateDesc.CS.pShaderBytecode = ComputeShaderBlob_->GetBufferPointer();
	computePipelineStateDesc.CS.BytecodeLength = ComputeShaderBlob_->GetBufferSize();
	computePipelineStateDesc.pRootSignature = ComputeRootSignature_.Get();

	HRESULT hr = dxCommon_->GetDevice()->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&ComputePipelineState_));
	assert(SUCCEEDED(hr));
}

void Skinning::CommonSettingCompute(SkinCluster skinCluster)
{
	// Compute用RootSignatureをセット
	dxCommon_->GetCommandList()->SetComputeRootSignature(ComputeRootSignature_.Get());

	// Compute用PipelineStateをセット
	dxCommon_->GetCommandList()->SetPipelineState(ComputePipelineState_.Get());


	dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(1, skinCluster.paletteSrvHandle.second);        // t0
	dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(2, skinCluster.inputVertexSrvHandle.second);    // t1
	dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(3, skinCluster.influenceSrvHandle.second);      // t2
	dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(4, skinCluster.outputVertexUavHandle.second);   // u0


	// 各種DescriptorとCBVを設定
	//dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(0, palette_.gpuHandle);        // 例: Palette (t0)
	//dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(1, inputVertex_.gpuHandle);    // 例: InputVertex (t1)
	//dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(2, influence_.gpuHandle);      // 例: Influence (t2)
	//dxCommon_->GetCommandList()->SetComputeRootDescriptorTable(3, outputVertex_.gpuHandle);   // 例: OutputVertex (u0)
	//dxCommon_->GetCommandList()->SetComputeRootConstantBufferView(4, skinningInformation_.buffer->GetGPUVirtualAddress()); // SkinningInfo (b0)
}

void Skinning::CommonSetting()
{
	//ルートシグネチャをセットするコマンド
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(RootSignature_.Get());

	//グラフィックスパイプラインをセットするコマンド
	dxCommon_->GetCommandList()->SetPipelineState(GraphicsPipelineState_.Get());    //PSOを設定

	//プリミティブトポロジーをセットするコマンド
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

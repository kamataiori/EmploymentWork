#include "Object3dCommon.h"
#include "Logger.h"

Object3dCommon* Object3dCommon::instance = nullptr;

Object3dCommon* Object3dCommon::GetInstance()
{
	if (!instance) {
		instance = new Object3dCommon();
	}
	return instance;
}

void Object3dCommon::Initialize()
{
	//引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();

	//グラフィックスパイプラインの生成
	GraphicsPipelineState();
}

void Object3dCommon::RootSignature()
{
	////=========DescriptorRange=========////

	//D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange_[0].BaseShaderRegister = 0;  //0から始まる
	descriptorRange_[0].NumDescriptors = 1;  //数は1
	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  //SRVを使う
	descriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  //Offsetを自動計算


	////=========RootSignatureを生成する=========////

	//RootSignature作成
	/*D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};*/
	descriptionRootSignature_.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	//RootSignature作成。複数設定できるので配列。今回は結果1つだけなので長さ1の配列
	/*D3D12_ROOT_PARAMETER rootParameters[4] = {};*/
	rootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
	rootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;    //PixelShaderで使う
	rootParameters_[0].Descriptor.ShaderRegister = 0;    //レジスタ番号0とバインド
	rootParameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;      //CBVを使う
	rootParameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;    //VertexShaderで使う
	rootParameters_[1].Descriptor.ShaderRegister = 0;      //レジスタ番号0を使う
	rootParameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  //DescriptorTableを使う
	rootParameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters_[2].DescriptorTable.pDescriptorRanges = descriptorRange_;
	rootParameters_[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange_);
	////=========平行光源をShaderで使う=========////
	rootParameters_[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[3].Descriptor.ShaderRegister = 1;  //レジスタ番号1を使う
	////=========光源のカメラの位置をShaderで使う=========////
	rootParameters_[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[4].Descriptor.ShaderRegister = 2;  //レジスタ番号1を使う
	////========ポイントライトをShaderで使う========////
	rootParameters_[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[5].Descriptor.ShaderRegister = 3;  //レジスタ番号3を使う

	////========スポットライトをShaderで使う========////
	rootParameters_[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[6].Descriptor.ShaderRegister = 4;  //レジスタ番号4を使う

	descriptionRootSignature_.pParameters = rootParameters_;    //ルートパラメータ配列へのポインタ
	descriptionRootSignature_.NumParameters = _countof(rootParameters_);    //配列の長さ


	////=========Samplerの設定=========////

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  //バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //0～1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  //比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;  //ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;  //レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	descriptionRootSignature_.pStaticSamplers = staticSamplers;
	descriptionRootSignature_.NumStaticSamplers = _countof(staticSamplers);



	////シリアライズしてバイナリにする
	//Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	/*Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;*/
	hr = dxCommon_->GetDevice().Get()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));

	if (FAILED(hr)) {
		Logger::Log("rootsig\n");
		exit(1);
	}

	assert(SUCCEEDED(hr));

}

void Object3dCommon::GraphicsPipelineState()
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

	vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Object3d.VS.hlsl",
		L"vs_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
	if (vertexShaderBlob_ == nullptr) {
		Logger::Log("vertexShaderBlob_\n");
		exit(1);
	}
	assert(vertexShaderBlob_ != nullptr);
	pixelShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Object3d.PS.hlsl",
		L"ps_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
	if (pixelShaderBlob_ == nullptr) {
		Logger::Log("pixelShaderBlob_\n");
		exit(1);
	}
	assert(pixelShaderBlob_ != nullptr);


	////=========DepthStencilStateの設定を行う=========////

	DepthStencilState();


	////=========PSOを生成する=========////

	PSO();
}

void Object3dCommon::InputLayout()
{
	////=========InputLayoutの設定を行う=========////

	/*D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};*/
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc_.pInputElementDescs = inputElementDescs;
	inputLayoutDesc_.NumElements = _countof(inputElementDescs);
}

void Object3dCommon::BlendState()
{
	////=========BlendStateの設定を行う=========////

	////BlendStateの設定
	//D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	//--------ノーマルブレンド--------//
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	//--------加算合成--------//
	/*blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/
	//--------減算合成--------//
	/*blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/
	//--------乗算合成--------//
	/*blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;*/
	//--------スクリーン合成--------//
	/*blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;*/
	//α値のブレンド設定で基本的には使わない
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
}

void Object3dCommon::RasterizerState()
{
	////=========RasterizerStateの設定を行う=========////

	//RasterizerStateの設定
	/*D3D12_RASTERIZER_DESC rasterizerDesc{};*/
	//裏面(時計回り)を表示しない
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	//三角形の中を塗りつぶす
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;
	//カリングしない(裏面も表示させる)
	//rasterizerDesc_.CullMode = D3D12_CULL_MODE_NONE;
}

void Object3dCommon::DepthStencilState()
{
	////=========DepthStencilStateの設定を行う=========////

	////DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc_.DepthEnable = true;
	//書き込みします
	depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

}

void Object3dCommon::PSO()
{
	////=========PSOを生成する=========////

	/*D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};*/
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();    //RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc_;    //InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(),
	vertexShaderBlob_->GetBufferSize() };    //VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(),
	pixelShaderBlob_->GetBufferSize() };    //PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc_;    //BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc_;    //RasterizerState
	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc_;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//実際に生成
	/*Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;*/
	HRESULT hr = dxCommon_->GetDevice().Get()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));

	if (FAILED(hr)) {
		Logger::Log("PSO\n");
		exit(1);
	}

	assert(SUCCEEDED(hr));
}

void Object3dCommon::Finalize()
{
	delete instance;
	instance = nullptr;
}

Camera* Object3dCommon::GetDefaultCamera()
{
	return defaultCamera;
}


void Object3dCommon::CommonSetting()
{
	//ルートシグネチャをセットするコマンド
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());

	//グラフィックスパイプラインをセットするコマンド
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());    //PSOを設定

	//プリミティブトポロジーをセットするコマンド
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
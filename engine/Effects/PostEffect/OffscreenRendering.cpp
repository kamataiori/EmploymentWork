#include "OffscreenRendering.h"
#include <Logger.h>
#include <SrvManager.h>

void OffscreenRendering::Initialize()
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();

	// RTV、SRVの作成
	//RenderTexture();

	// グラフィックスパイプラインの生成
	GraphicsPipelineState();
}

void OffscreenRendering::Update()
{
}

void OffscreenRendering::Draw()
{
	// ルートシグネチャの設定
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());

	// パイプラインステートの設定
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

	// トポロジの設定
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//srvIndex_ = SrvManager::GetInstance()->Allocate();
	////dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvIndex_);

	//SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(0, srvIndex_);

	// DescriptorTable（SRV）の設定
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle =
		SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex_);
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvHandle);

	// 描画
	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OffscreenRendering::RenderTexture()
{
	// RenderTextureを使って、そこにSceneを描画していくようにするので、RTVも作成する
	// 今回はSwapChainと同じにする

	//--------RTVの作成--------//
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// レンダーテクスチャの RTV ハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE renderTextureRTVHandle_ = dxCommon_->GetRenderTextureRTVHandle();

	const Vector4 kREnderTargetClearValue{ 1.0f,0.0f,0.0f,1.0f };  // 一旦分かりやすいように赤色に設定
	auto renderTextureResource = dxCommon_->CreateRenderTextureResource(dxCommon_->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kREnderTargetClearValue);
	dxCommon_->GetDevice()->CreateRenderTargetView(renderTextureResource.Get(), &rtvDesc, renderTextureRTVHandle_);


	//--------SRVの作成--------//

	//// SRVの設定。FormatはResourceと同じにしておく
	//D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
	//renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//renderTextureSrvDesc.Texture2D.MipLevels = 1;

	//// SRVの生成
	//dxCommon_->GetDevice()->CreateShaderResourceView(
	//	renderTextureResource.Get(),
	//	&renderTextureSrvDesc,
	//	dxCommon_->GetCPUDescriptorHandle(SrvManager::GetInstance()->GetSrvDescriptorHeap().Get(), SrvManager::GetInstance()->GetDescriptorSizeSRV(), 10) // 適切なハンドルを取得
	//);

	srvIndex_ = SrvManager::GetInstance()->Allocate();

	SrvManager::GetInstance()->CreateSRVforTexture2D(srvIndex_, renderTextureResource.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);


}

//void OffscreenRendering::RootSignature()
//{
//	////=========DescriptorRange=========////
//
//	descriptorRange_[0].BaseShaderRegister = 0;  //0から始まる
//	descriptorRange_[0].NumDescriptors = 1;  //数は1
//	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  //SRVを使う
//	descriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  //Offsetを自動計算
//
//
//	////=========RootSignatureを生成する=========////
//
//	// RootSignature作成
//	descriptionRootSignature_.Flags =
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
//
//
//	// RootSignature作成。複数設定できるので配列。
//	rootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
//	rootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;    //PixelShaderで使う
//	rootParameters_[0].Descriptor.ShaderRegister = 0;    //レジスタ番号0とバインド
//	rootParameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;      //CBVを使う
//	rootParameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;    //VertexShaderで使う
//	rootParameters_[1].Descriptor.ShaderRegister = 0;      //レジスタ番号0を使う
//	rootParameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  //DescriptorTableを使う
//	rootParameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//	rootParameters_[2].DescriptorTable.pDescriptorRanges = descriptorRange_;
//	rootParameters_[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange_);
//
//	descriptionRootSignature_.pParameters = rootParameters_;    //ルートパラメータ配列へのポインタ
//	descriptionRootSignature_.NumParameters = _countof(rootParameters_);    //配列の長さ
//
//
//	////=========Samplerの設定=========////
//
//	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
//	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  //バイリニアフィルタ
//	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //0～1の範囲外をリピート
//	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  //比較しない
//	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;  //ありったけのMipmapを使う
//	staticSamplers[0].ShaderRegister = 0;  //レジスタ番号0を使う
//	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
//	descriptionRootSignature_.pStaticSamplers = staticSamplers;
//	descriptionRootSignature_.NumStaticSamplers = _countof(staticSamplers);
//
//
//	// シリアライズしてバイナリにする
//	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature_,
//		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
//	if (FAILED(hr)) {
//		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
//		assert(false);
//	}
//	// バイナリを元に生成
//	hr = dxCommon_->GetDevice().Get()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
//		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
//	assert(SUCCEEDED(hr));
//}

void OffscreenRendering::RootSignature()
{
	// DescriptorRangeの設定
	descriptorRange_[0].BaseShaderRegister = 0;  // SRVのベースレジスタ
	descriptorRange_[0].NumDescriptors = 1;     // 使用するSRVの数
	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  // SRVを使用
	descriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameterの設定 (DescriptorTable)
	rootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters_[0].DescriptorTable.pDescriptorRanges = descriptorRange_;
	rootParameters_[0].DescriptorTable.NumDescriptorRanges = 1;

	// RootSignatureのフラグ
	descriptionRootSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootSignatureの構成
	descriptionRootSignature_.pParameters = rootParameters_;
	descriptionRootSignature_.NumParameters = 1;  // パラメータは1つのみ

	// サンプラーの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature_.pStaticSamplers = staticSamplers;
	descriptionRootSignature_.NumStaticSamplers = 1;

	// シリアライズしてバイナリ化
	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionRootSignature_, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	// RootSignatureの作成
	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}


void OffscreenRendering::GraphicsPipelineState()
{
	// ルートシグネチャの生成
	RootSignature();

	////=========InputLayoutの設定を行う=========////

	InputLayout();


	////=========BlendStateの設定を行う=========////

	BlendState();

	////=========RasterizerStateの設定を行う=========////

	RasterizerState();


	////=========ShaderをCompileする=========////

	vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl",
		L"vs_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
	if (vertexShaderBlob_ == nullptr) {
		Logger::Log("vertexShaderBlob_\n");
		exit(1);
	}
	assert(vertexShaderBlob_ != nullptr);
	pixelShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/VignetteRed.PS.hlsl",
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

void OffscreenRendering::InputLayout()
{
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

	// 頂点には何もデータを入力しないので、InputLayoutは利用しない。ドライバやGPUのやることが
	// 少なくなりそうな気配を感じる
	inputLayoutDesc_.pInputElementDescs = nullptr; /*inputElementDescs;*/
	inputLayoutDesc_.NumElements = 0; /*_countof(inputElementDescs);*/
}

void OffscreenRendering::BlendState()
{
	// すべての色要素を書き込む
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
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
	// α値のブ_レンド設定で基本的には使わない
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
}

void OffscreenRendering::RasterizerState()
{
	// 裏面(時計回り)を表示しない
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;
	// カリングしない(裏面も表示させる)
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_NONE;
}

void OffscreenRendering::DepthStencilState()
{
	// 全画面に対してなにか処理をほどこしたいだけだから、比較も書き込みも必要ないのでDepth自体不要
	depthStencilDesc_.DepthEnable = true;
	// 書き込みします
	depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

void OffscreenRendering::PSO()
{
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();    //RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc_;    //InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(),
	vertexShaderBlob_->GetBufferSize() };    //VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(),
	pixelShaderBlob_->GetBufferSize() };    //PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc_;    //BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc_;    //RasterizerState
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc_;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 実際に生成
	HRESULT hr = dxCommon_->GetDevice().Get()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));

	if (FAILED(hr)) {
		Logger::Log("PSO\n");
		exit(1);
	}

	assert(SUCCEEDED(hr));
}
#include "OffscreenRendering.h"
#include <Logger.h>
#include <SrvManager.h>
#include <externals/DirectXTex/d3dx12.h>
#include <TextureManager.h>

void OffscreenRendering::Initialize(PostEffectType type)
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();

	// RTV、SRVの作成
	//RenderTexture();

	// グラフィックスパイプラインの生成
	CreateAllRootSignatures();
	CreateAllPSOs();  // 全PSOを一括生成
	//currentType_ = type;
	graphicsPipelineState = pipelineStates_[static_cast<size_t>(type)];

	// 各エフェクトの初期化
	VignetteInitialize(16.0f, 0.2f, { 1.0f,1.0f,0.0f });
	GrayscaleInitialize(1.0f, { 0.2125f, 0.7154f, 0.0721f });
	SepiaInitialize({ 1.0f, 74.0f / 107.0f, 43.0f / 107.0f}, 0.9f);
	RadialBlurInitialize({ 0.5f,0.5f }, 0.01f, 10);
	RandomInitialize(1.0f,100.0f,0.5f);
	DissolveInitialize(0.3f,0.3f,{0.2f,0.4f,0.5f});
}

void OffscreenRendering::Update()
{
}

void OffscreenRendering::Draw()
{
	ID3D12GraphicsCommandList* cmd = dxCommon_->GetCommandList().Get();
	size_t i = static_cast<size_t>(currentType_);

	cmd->SetPipelineState(pipelineStates_[i].Get());
	cmd->SetGraphicsRootSignature(rootSignatures_[i].Get());

	cmd->SetGraphicsRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex_));

	if (currentType_ == PostEffectType::Dissolve) {
		// Dissolve: gTexture (t0), gMaskTexture (t1)
		auto sceneHandle = TextureManager::GetInstance()->GetSrvHandleGPU(dissolveScenePath_);
		auto noiseHandle = TextureManager::GetInstance()->GetSrvHandleGPU(dissolveNoisePath_);

		// SRV t0 と t1 をそれぞれ RootParameter で渡す
		cmd->SetGraphicsRootDescriptorTable(0, sceneHandle); // gTexture
		cmd->SetGraphicsRootDescriptorTable(1, noiseHandle); // gMaskTexture

		// CBV: Dissolve の threshold
		cmd->SetGraphicsRootConstantBufferView(2, dissolveCB_->GetGPUVirtualAddress());
	}
	else {
		// 通常のポストエフェクト
		cmd->SetGraphicsRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex_));

		if (currentType_ == PostEffectType::Vignette && vignetteCB_) {
			cmd->SetGraphicsRootConstantBufferView(1, vignetteCB_->GetGPUVirtualAddress());
		}
		else if (currentType_ == PostEffectType::Grayscale && grayscaleCB_) {
			cmd->SetGraphicsRootConstantBufferView(1, grayscaleCB_->GetGPUVirtualAddress());
		}
		else if (currentType_ == PostEffectType::Sepia && sepiaCB_) {
			cmd->SetGraphicsRootConstantBufferView(1, sepiaCB_->GetGPUVirtualAddress());
		}
		else if (currentType_ == PostEffectType::RadialBlur && radialBlurCB_) {
			cmd->SetGraphicsRootConstantBufferView(1, radialBlurCB_->GetGPUVirtualAddress());
		}
		else if (currentType_ == PostEffectType::Random && randomCB_) {
			cmd->SetGraphicsRootConstantBufferView(1, randomCB_->GetGPUVirtualAddress());
		}
	}


	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void OffscreenRendering::SetPostEffectType(PostEffectType type)
{
	if (currentType_ == type) return;  // 同じならスキップ
	currentType_ = type;
	graphicsPipelineState = pipelineStates_[static_cast<size_t>(type)];
}

PostEffectType OffscreenRendering::GetPostEffectType() const
{
	return currentType_;
}

void OffscreenRendering::VignetteInitialize(float scale, float power, const Vector3& color)
{
	if (!vignetteCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(VignetteCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&vignetteCB_));
		assert(SUCCEEDED(hr));
		vignetteCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVignetteCB_));
	}
	mappedVignetteCB_->vignetteScale = scale;
	mappedVignetteCB_->vignettePower = power;
	mappedVignetteCB_->vignetteColor = color;
}

void OffscreenRendering::SetVignetteScale(float scale)
{
	if (mappedVignetteCB_) {
		mappedVignetteCB_->vignetteScale = scale;
	}
}

void OffscreenRendering::SetVignettePower(float power)
{
	if (mappedVignetteCB_) {
		mappedVignetteCB_->vignettePower = power;
	}
}

void OffscreenRendering::SetVignetteColor(const Vector3& color)
{
	if (mappedVignetteCB_) {
		mappedVignetteCB_->vignetteColor.x = color.x;
		mappedVignetteCB_->vignetteColor.y = color.y;
		mappedVignetteCB_->vignetteColor.z = color.z;
	}
}

void OffscreenRendering::GrayscaleInitialize(float strength, const Vector3& weights)
{
	if (!grayscaleCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(GrayscaleCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&grayscaleCB_));
		assert(SUCCEEDED(hr));
		grayscaleCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedGrayscaleCB_));
	}
	mappedGrayscaleCB_->strength = strength;
	mappedGrayscaleCB_->weights = weights;
}

void OffscreenRendering::SetGrayscaleStrength(float strength)
{
	if (mappedGrayscaleCB_) {
		mappedGrayscaleCB_->strength = strength;
	}
}

void OffscreenRendering::SetGrayscaleWeights(const Vector3& weights)
{
	if (mappedGrayscaleCB_) {
		mappedGrayscaleCB_->weights = weights;
	}
}

void OffscreenRendering::SepiaInitialize(const Vector3& sepiaColor, float strength)
{
	if (!sepiaCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SepiaCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&sepiaCB_));
		assert(SUCCEEDED(hr));
		sepiaCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedSepiaCB_));
	}
	mappedSepiaCB_->sepiaColor = sepiaColor;
	mappedSepiaCB_->sepiaStrength = strength;
}

void OffscreenRendering::SetSepiaColor(const Vector3& sepiaColor)
{
	if (mappedSepiaCB_) {
		mappedSepiaCB_->sepiaColor = sepiaColor;
	}
}

void OffscreenRendering::SetSepiaStrength(float strength)
{
	if (mappedSepiaCB_) {
		mappedSepiaCB_->sepiaStrength = strength;
	}
}

void OffscreenRendering::RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples)
{
	if (!radialBlurCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(RadialBlurCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&radialBlurCB_));
		assert(SUCCEEDED(hr));
		radialBlurCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedRadialBlurCB_));
	}
	mappedRadialBlurCB_->center = center;
	mappedRadialBlurCB_->blurWidth = blurWidth;
	mappedRadialBlurCB_->numSamples = numSamples;
}

void OffscreenRendering::SetRadialBlurCenter(const Vector2& center)
{
	if (mappedRadialBlurCB_) {
		mappedRadialBlurCB_->center = center;
	}
}

void OffscreenRendering::SetRadialBlurWidth(float blurWidth)
{
	if (mappedRadialBlurCB_) {
		mappedRadialBlurCB_->blurWidth = blurWidth;
	}
}

void OffscreenRendering::SetRadialBlurSamples(int numSamples)
{
	if (mappedRadialBlurCB_) {
		mappedRadialBlurCB_->numSamples = numSamples;
	}
}

void OffscreenRendering::RandomInitialize(float time, float scale, float intensity)
{
	if (!randomCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(RandomCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&randomCB_));
		assert(SUCCEEDED(hr));
		randomCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedRandomCB_));
	}
	mappedRandomCB_->time = time;
	mappedRandomCB_->scale = scale;
	mappedRandomCB_->intensity = intensity;
	/*mappedRandomCB_->dummy = 0.0f;*/
}

void OffscreenRendering::SetRandomTime(float time)
{
	if (mappedRandomCB_) {
		mappedRandomCB_->time = time;
	}
}

void OffscreenRendering::SetRandomScale(float scale)
{
	if (mappedRandomCB_) {
		mappedRandomCB_->scale = scale;
	}
}

void OffscreenRendering::SetRandomIntensity(float intensity)
{
	if (mappedRandomCB_) {
		mappedRandomCB_->intensity = intensity;
	}
}

void OffscreenRendering::SetRandomUseImage(bool useImage)
{
	if (mappedRandomCB_) {
		mappedRandomCB_->useImage = useImage ? 1.0f : 0.0f;
	}
}

void OffscreenRendering::DissolveInitialize(float threshold, float edgeWidth, const Vector3& edgeColor) {
	if (!dissolveCB_) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(DissolveCB) + 0xFF) & ~0xFF);
		HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&dissolveCB_));
		assert(SUCCEEDED(hr));
		dissolveCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedDissolveCB_));
	}
	mappedDissolveCB_->threshold = threshold;
	mappedDissolveCB_->edgeWidth = edgeWidth;
	mappedDissolveCB_->edgeColor = edgeColor;
}

void OffscreenRendering::SetDissolveThreshold(float value) {
	if (mappedDissolveCB_) mappedDissolveCB_->threshold = value;
}
void OffscreenRendering::SetDissolveEdgeWidth(float value) {
	if (mappedDissolveCB_) mappedDissolveCB_->edgeWidth = value;
}
void OffscreenRendering::SetDissolveEdgeColor(const Vector3& color) {
	if (mappedDissolveCB_) mappedDissolveCB_->edgeColor = color;
}

void OffscreenRendering::SetDissolveTextures(const std::string& scenePath, const std::string& noisePath) {
	TextureManager::GetInstance()->LoadTexture(scenePath);
	TextureManager::GetInstance()->LoadTexture(noisePath);
	dissolveScenePath_ = scenePath;
	dissolveNoisePath_ = noisePath;
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

	srvIndex_ = SrvManager::GetInstance()->Allocate();

	SrvManager::GetInstance()->CreateSRVforTexture2D(srvIndex_, renderTextureResource.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);


}

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


//void OffscreenRendering::GraphicsPipelineState(PostEffectType type)
//{
//	// ルートシグネチャの生成
//	RootSignature();
//
//	////=========InputLayoutの設定を行う=========////
//
//	InputLayout();
//
//
//	////=========BlendStateの設定を行う=========////
//
//	BlendState();
//
//	////=========RasterizerStateの設定を行う=========////
//
//	RasterizerState();
//
//
//	////=========ShaderをCompileする=========////
//
//	vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl",
//		L"vs_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
//	if (vertexShaderBlob_ == nullptr) {
//		Logger::Log("vertexShaderBlob_\n");
//		exit(1);
//	}
//	assert(vertexShaderBlob_ != nullptr);
//	pixelShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/CopyImage.PS.hlsl",
//		L"ps_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
//	if (pixelShaderBlob_ == nullptr) {
//		Logger::Log("pixelShaderBlob_\n");
//		exit(1);
//	}
//	assert(pixelShaderBlob_ != nullptr);
//
//
//	////=========DepthStencilStateの設定を行う=========////
//
//	DepthStencilState();
//
//
//	////=========PSOを生成する=========////
//
//	PSO();
//}

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

void OffscreenRendering::CreateAllPSOs()
{
	vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");

	struct PostEffectShader {
		PostEffectType type;
		std::wstring psPath;
	};

	std::vector<PostEffectShader> shaders = {
		{ PostEffectType::Normal, L"Resources/shaders/CopyImage.PS.hlsl" },
		/*{ PostEffectType::Blur5x5, L"Resources/shaders/BoxFilter.PS.hlsl" },
		{ PostEffectType::Blur3x3, L"Resources/shaders/BoxFilter3x3.PS.hlsl" },
		{ PostEffectType::GaussianFilter , L"Resources/shaders/GaussianFilter.PS.hlsl" },*/
		{ PostEffectType::RadialBlur , L"Resources/shaders/RadialBlur.PS.hlsl" },
		{ PostEffectType::Grayscale, L"Resources/shaders/Grayscale.PS.hlsl" },
		{ PostEffectType::Vignette, L"Resources/shaders/Vignette.PS.hlsl" },
		{ PostEffectType::Sepia, L"Resources/shaders/Sepia.PS.hlsl" },
		{ PostEffectType::Random, L"Resources/shaders/Random.PS.hlsl" },
		{ PostEffectType::Dissolve, L"Resources/shaders/Dissolve.PS.hlsl" },
	};

	//vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");

	for (const auto& shader : shaders) {
		auto ps = dxCommon_->CompileShader(shader.psPath.c_str(), L"ps_6_0");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = rootSignatures_[static_cast<size_t>(shader.type)].Get();
		desc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
		desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.InputLayout = { nullptr, 0 };
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleDesc.Count = 1;

		HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineStates_[static_cast<size_t>(shader.type)]));
		assert(SUCCEEDED(hr));
	}
}

void OffscreenRendering::CreateAllRootSignatures()
{
	for (size_t i = 0; i < kPostEffectCount; ++i) {
		PostEffectType type = static_cast<PostEffectType>(i);

		std::vector<CD3DX12_ROOT_PARAMETER> params;
		CD3DX12_ROOT_SIGNATURE_DESC desc;

		if (type == PostEffectType::Dissolve) {
			// SRV2つ: gTexture (t0), gMaskTexture (t1)
			CD3DX12_DESCRIPTOR_RANGE range0;
			range0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
			CD3DX12_DESCRIPTOR_RANGE range1;
			range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1

			CD3DX12_ROOT_PARAMETER srvParam0;
			srvParam0.InitAsDescriptorTable(1, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
			params.push_back(srvParam0);

			CD3DX12_ROOT_PARAMETER srvParam1;
			srvParam1.InitAsDescriptorTable(1, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
			params.push_back(srvParam1);

			CD3DX12_ROOT_PARAMETER cbvParam;
			cbvParam.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
			params.push_back(cbvParam);

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.ShaderRegister = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			desc.Init((UINT)params.size(), params.data(), 1, &sampler,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		}
		else {
			CD3DX12_DESCRIPTOR_RANGE range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

			CD3DX12_ROOT_PARAMETER srvParam;
			srvParam.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
			params.push_back(srvParam);

			if (type == PostEffectType::Vignette || type == PostEffectType::Grayscale ||
				type == PostEffectType::Sepia || type == PostEffectType::RadialBlur ||
				type == PostEffectType::Random) {

				CD3DX12_ROOT_PARAMETER cbvParam;
				cbvParam.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
				params.push_back(cbvParam);
			}

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.ShaderRegister = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			desc.Init((UINT)params.size(), params.data(), 1, &sampler,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		}

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		assert(SUCCEEDED(hr));

		hr = dxCommon_->GetDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatures_[i]));
		assert(SUCCEEDED(hr));
	}
}

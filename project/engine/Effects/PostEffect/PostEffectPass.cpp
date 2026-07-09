#include "PostEffectPass.h"
#include <Logger.h>
#include <SrvManager.h>
#include <TextureManager.h>
#include <vector>

//========================================================================
// PostEffectPassBase
//========================================================================
void PostEffectPassBase::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;
	CreateStandardRootSignature(UseConstantBuffer());
	CreatePipeline(GetPixelShaderPath());
	OnInitialized();
}

void PostEffectPassBase::CreateStandardRootSignature(bool useCbv)
{
	std::vector<CD3DX12_ROOT_PARAMETER> params{};

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// SRV(t0) のディスクリプタテーブル
	CD3DX12_DESCRIPTOR_RANGE range = {};
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
	CD3DX12_ROOT_PARAMETER srvParam = {};
	srvParam.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
	params.push_back(srvParam);

	if (useCbv) {
		CD3DX12_ROOT_PARAMETER cbvParam = {};
		cbvParam.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // b0
		params.push_back(cbvParam);
	}

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(static_cast<UINT>(params.size()), params.data(), 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob{};
	Microsoft::WRL::ComPtr<ID3DBlob> errBlob{};
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	assert(SUCCEEDED(hr));
	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void PostEffectPassBase::CreatePipeline(const std::wstring& psPath)
{
	auto vs = dxCommon_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
	assert(vs != nullptr);
	auto ps = dxCommon_->CompileShader(psPath.c_str(), L"ps_6_0");
	assert(ps != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = rootSignature_.Get();
	desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
	desc.BlendState = GetBlendDesc();
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// ポストエフェクト／合成のフルスクリーンパスは深度不要。
	// 深度を有効にすると、複数のフルスクリーンパスが同じ深度に書き込み合い
	// 後続パスが DepthFunc(LESS) で自己棄却されてしまう（パーティクル層が消える等）。
	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.InputLayout = { nullptr, 0 };
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		Logger::Log("PostEffectPass PSO creation failed\n");
	}
	assert(SUCCEEDED(hr));
}

void PostEffectPassBase::BindCommon(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetGraphicsRootDescriptorTable(
		0, SrvManager::GetInstance()->GetGPUDescriptorHandle(inputSrvIndex));
}

void PostEffectPassBase::DrawFullscreen(ID3D12GraphicsCommandList* commandList)
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void PostEffectPassBase::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	DrawFullscreen(commandList);
}

//========================================================================
// VignettePass
//========================================================================
void VignettePass::SetParams(float scale, float power, const Vector3& color)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->scale = scale;
	mapped_->power = power;
	mapped_->color = color;
}

void VignettePass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

//========================================================================
// GrayscalePass
//========================================================================
void GrayscalePass::SetParams(float strength, const Vector3& weights)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->strength = strength;
	mapped_->weights = weights;
}

void GrayscalePass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

//========================================================================
// SepiaPass
//========================================================================
void SepiaPass::SetParams(const Vector3& color, float strength)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->color = color;
	mapped_->strength = strength;
}

void SepiaPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

//========================================================================
// RadialBlurPass
//========================================================================
void RadialBlurPass::SetParams(const Vector2& center, float blurWidth, int numSamples)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->center = center;
	mapped_->blurWidth = blurWidth;
	mapped_->numSamples = numSamples;
}

void RadialBlurPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

//========================================================================
// RandomPass
//========================================================================
void RandomPass::SetParams(float time, float scale, float intensity)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->time = time;
	mapped_->scale = scale;
	mapped_->intensity = intensity;
}

void RandomPass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex)
{
	BindCommon(commandList, inputSrvIndex);
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(1, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

//========================================================================
// DissolvePass
//========================================================================
void DissolvePass::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;
	CreateDissolveRootSignature();
	CreatePipeline(GetPixelShaderPath());
}

void DissolvePass::CreateDissolveRootSignature()
{
	std::vector<CD3DX12_ROOT_PARAMETER> params{};

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// gTexture (t0)
	CD3DX12_DESCRIPTOR_RANGE range0 = {};
	range0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_ROOT_PARAMETER srvParam0 = {};
	srvParam0.InitAsDescriptorTable(1, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
	params.push_back(srvParam0);

	// gMaskTexture (t1)
	CD3DX12_DESCRIPTOR_RANGE range1 = {};
	range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	CD3DX12_ROOT_PARAMETER srvParam1 = {};
	srvParam1.InitAsDescriptorTable(1, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
	params.push_back(srvParam1);

	// CBV (b0)
	CD3DX12_ROOT_PARAMETER cbvParam = {};
	cbvParam.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	params.push_back(cbvParam);

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(static_cast<UINT>(params.size()), params.data(), 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob{};
	Microsoft::WRL::ComPtr<ID3DBlob> errBlob{};
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	assert(SUCCEEDED(hr));
	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void DissolvePass::SetParams(float threshold, float edgeWidth, const Vector3& edgeColor)
{
	if (!cb_) {
		CreateConstantBuffer(cb_, mapped_);
	}
	mapped_->threshold = threshold;
	mapped_->edgeWidth = edgeWidth;
	mapped_->edgeColor = edgeColor;
}

void DissolvePass::SetTextures(const std::string& scenePath, const std::string& noisePath)
{
	TextureManager::GetInstance()->LoadTexture(scenePath);
	TextureManager::GetInstance()->LoadTexture(noisePath);
	scenePath_ = scenePath;
	noisePath_ = noisePath;
}

void DissolvePass::Execute(ID3D12GraphicsCommandList* commandList, uint32_t /*inputSrvIndex*/)
{
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	auto sceneHandle = TextureManager::GetInstance()->GetSrvHandleGPU(scenePath_);
	auto noiseHandle = TextureManager::GetInstance()->GetSrvHandleGPU(noisePath_);
	commandList->SetGraphicsRootDescriptorTable(0, sceneHandle); // gTexture
	commandList->SetGraphicsRootDescriptorTable(1, noiseHandle); // gMaskTexture
	if (cb_) {
		commandList->SetGraphicsRootConstantBufferView(2, cb_->GetGPUVirtualAddress());
	}
	DrawFullscreen(commandList);
}

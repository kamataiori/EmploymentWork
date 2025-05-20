#include "SkyBox.h"
#include <cassert>
#include <externals/DirectXTex/d3dx12.h>
#include <Object3dCommon.h>
#include <TextureManager.h>

using namespace DirectX;

struct ConstBufferData {
    XMMATRIX wvp;
};

void SkyBox::Initialize()
{
    dxCommon_ = DirectXCommon::GetInstance();
    /*CreateVertexData();
    CreateIndexData();*/
    CreateBuffers();
    CreateMaterialData();
    //CreateTransformationMatrixData();
    RootSignature();
    GraphicsPipelineState();

    textureFilePath_ = "Resources/rostock_laage_airport_4k.dds";
    TextureManager::GetInstance()->LoadTexture(textureFilePath_);

    transform = { {100.0f,100.0f,100.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
}

void SkyBox::Update()
{

    Matrix4x4 mat = camera_->GetViewMatrix();
    XMFLOAT4X4 viewFloat;
    memcpy(&viewFloat, &mat, sizeof(Matrix4x4));
    viewFloat.m[3][0] = 0.0f;
    viewFloat.m[3][1] = 0.0f;
    viewFloat.m[3][2] = 0.0f;


    XMMATRIX view = XMLoadFloat4x4(&viewFloat);
    XMMATRIX proj = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&camera_->GetProjectionMatrix()));
    XMMATRIX world = XMMatrixIdentity();
    Matrix4x4 mat4world = MakeAffineMatrix(transform.scale,transform.rotate,transform.translate);
    world = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat4world));

    ConstBufferData* cbData = reinterpret_cast<ConstBufferData*>(mappedConst_);
    cbData->wvp = world * view * proj;
}

void SkyBox::Draw()
{
    ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList().Get();

    cmdList->SetGraphicsRootSignature(skyboxRootSignature_.Get());
    cmdList->SetPipelineState(skyboxPipelineState_.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    cmdList->IASetIndexBuffer(&indexBufferView_);
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());
    /*cmdList->SetGraphicsRootConstantBufferView(1, materialBuffer_->GetGPUVirtualAddress());*/
    cmdList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

    cmdList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void SkyBox::CreateVertexData()
{
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 texcoord; // CubeMap は方向ベクトル（float3）を使う
    };


    std::vector<Vertex> vertexData = {
    {{ 1, 1, 1 },   { 1, 1, 1 }},
    {{ 1, 1,-1 },   { 1, 1,-1 }},
    {{ 1,-1,-1 },   { 1,-1,-1 }},
    {{ 1,-1, 1 },   { 1,-1, 1 }},

    {{-1, 1,-1 },   {-1, 1,-1 }},
    {{-1, 1, 1 },   {-1, 1, 1 }},
    {{-1,-1, 1 },   {-1,-1, 1 }},
    {{-1,-1,-1 },   {-1,-1,-1 }},

    {{-1, 1, 1 },   {-1, 1, 1 }},
    {{ 1, 1, 1 },   { 1, 1, 1 }},
    {{ 1,-1, 1 },   { 1,-1, 1 }},
    {{-1,-1, 1 },   {-1,-1, 1 }},

    {{ 1, 1,-1 },   { 1, 1,-1 }},
    {{-1, 1,-1 },   {-1, 1,-1 }},
    {{-1,-1,-1 },   {-1,-1,-1 }},
    {{ 1,-1,-1 },   { 1,-1,-1 }},

    {{-1, 1,-1 },   {-1, 1,-1 }},
    {{ 1, 1,-1 },   { 1, 1,-1 }},
    {{ 1, 1, 1 },   { 1, 1, 1 }},
    {{-1, 1, 1 },   {-1, 1, 1 }},

    {{-1,-1, 1 },   {-1,-1, 1 }},
    {{ 1,-1, 1 },   { 1,-1, 1 }},
    {{ 1,-1,-1 },   { 1,-1,-1 }},
    {{-1,-1,-1 },   {-1,-1,-1 }},
    };


    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * vertexData.size());

    dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));

    void* mapped = nullptr;
    vertexBuffer_->Map(0, nullptr, &mapped);
    memcpy(mapped, vertexData.data(), sizeof(Vertex) * vertexData.size());
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(Vertex);
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertexData.size());
}

void SkyBox::CreateIndexData()
{
    indices_ = {
        0,1,2, 2,3,0,    4,5,6, 6,7,4,    8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };
    indexCount_ = static_cast<uint32_t>(indices_.size());

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t) * indices_.size());

    dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer_));

    void* mapped = nullptr;
    indexBuffer_->Map(0, nullptr, &mapped);
    memcpy(mapped, indices_.data(), sizeof(uint32_t) * indices_.size());
    indexBuffer_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices_.size());
}

void SkyBox::CreateMaterialData() {}

void SkyBox::CreateTransformationMatrixData()
{
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff);

    dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer_));

    constantBuffer_->Map(0, nullptr, &mappedConst_);
}

void SkyBox::RootSignature()
{
    ID3D12Device* device = dxCommon_->GetDevice().Get();

    CD3DX12_DESCRIPTOR_RANGE rangeCBV[2];
    rangeCBV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
    rangeCBV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // b1

    CD3DX12_DESCRIPTOR_RANGE rangeSRV;
    rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

    CD3DX12_ROOT_PARAMETER rootParams[2];
    // b0: TransformationMatrix（VS用）
    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // b1: Material（PS用）
    //rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // t0: Texture（SRV）
    /*CD3DX12_DESCRIPTOR_RANGE rangeSRV;*/
    rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParams[1].InitAsDescriptorTable(1, &rangeSRV, D3D12_SHADER_VISIBILITY_PIXEL);


    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = _countof(rootParams);
    desc.pParameters = rootParams;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));

    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&skyboxRootSignature_));
    assert(SUCCEEDED(hr));
}

void SkyBox::GraphicsPipelineState()
{
    ID3D12Device* device = dxCommon_->GetDevice().Get();
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = dxCommon_->CompileShader(L"Resources/shaders/Skybox.VS.hlsl", L"vs_6_0").Get();
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob = dxCommon_->CompileShader(L"Resources/shaders/Skybox.PS.hlsl", L"ps_6_0").Get();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    D3D12_INPUT_ELEMENT_DESC layout[] = {
     { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
     { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    desc.InputLayout = { layout, _countof(layout) };

    desc.pRootSignature = skyboxRootSignature_.Get();
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    //desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleDesc.Count = 1;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&skyboxPipelineState_));
    assert(SUCCEEDED(hr));
}

void SkyBox::CreateBuffers()
{
    CreateVertexData();
    CreateIndexData();
    CreateTransformationMatrixData();
}

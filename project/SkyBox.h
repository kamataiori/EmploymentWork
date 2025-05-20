#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <DirectXMath.h>
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Matrix4x4.h"

class SkyBox
{
public:
    void Initialize();
    void Update();
    void Draw();

    void SetCamera(Camera* camera) { camera_ = camera; }

private:
    void CreateVertexData();
    void CreateIndexData();
    void CreateMaterialData();
    void CreateTransformationMatrixData();
    void RootSignature();
    void GraphicsPipelineState();
    void CreateBuffers();

private:
    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> skyboxRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> skyboxPipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    void* mappedConst_ = nullptr;

    std::vector<uint32_t> indices_;
    uint32_t indexCount_ = 0;

    // キューブマップのファイルパス
    std::string textureFilePath_;

    Camera* camera_{};

    Transform transform;
};
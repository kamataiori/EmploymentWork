#include "DrawTriangle.h"

DrawTriangle* DrawTriangle::instance = nullptr;

DrawTriangle* DrawTriangle::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new DrawTriangle;
    }
    return instance;
}

void DrawTriangle::Finalize()
{
    delete instance;
    instance = nullptr;
}

void DrawTriangle::Initialize()
{
    dxCommon_ = DirectXCommon::GetInstance();
    drawTriangleCommon_ = DrawTriangleCommon::GetInstance();

    transformationMatrixResource_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    *transformationMatrixData_ = { MakeIdentity4x4(), MakeIdentity4x4() };
}

void DrawTriangle::AddTriangle(const Vector3& p1, const Vector3& p2, const Vector3& p3, Color color, float alpha)
{
    uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());

    DirectX::XMFLOAT4 colorValue = GetColorValue(color);
    colorValue.w = alpha; // 透過度を設定

    vertices_.push_back({ {p1.x, p1.y, p1.z}, {colorValue.x, colorValue.y, colorValue.z, colorValue.w} });
    vertices_.push_back({ {p2.x, p2.y, p2.z}, {colorValue.x, colorValue.y, colorValue.z, colorValue.w} });
    vertices_.push_back({ {p3.x, p3.y, p3.z}, {colorValue.x, colorValue.y, colorValue.z, colorValue.w} });

    indices_.push_back(baseIndex);
    indices_.push_back(baseIndex + 1);
    indices_.push_back(baseIndex + 2);
}


void DrawTriangle::Update() {
    if (!camera_) return;

    transformationMatrixData_->World = MakeIdentity4x4();
    transformationMatrixData_->WVP = Multiply(transformationMatrixData_->World, camera_->GetViewProjectionMatrix());
}

void DrawTriangle::Draw() {
    if (vertices_.empty() || !camera_) return;

    if (vertices_.size() > 0)
    {
        const size_t vertexBufferSize = vertices_.size() * sizeof(DrawTriangleCommon::Vertex);
        const size_t indexBufferSize = indices_.size() * sizeof(uint32_t);

        vertexResource_ = dxCommon_->CreateBufferResource(vertexBufferSize);
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
        memcpy(vertexData_, vertices_.data(), vertexBufferSize);
        vertexResource_->Unmap(0, nullptr);

        indexResource_ = dxCommon_->CreateBufferResource(indexBufferSize);
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
        memcpy(indexData_, indices_.data(), indexBufferSize);
        indexResource_->Unmap(0, nullptr);

        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
        vertexBufferView_.StrideInBytes = sizeof(DrawTriangleCommon::Vertex);

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
        dxCommon_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
        dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, transformationMatrixResource_->GetGPUVirtualAddress());

        dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
    }
}

size_t DrawTriangle::GetTriangleCount() const
{
    return vertices_.size() / 3;
}

void DrawTriangle::ResetData() {
    vertices_.clear();
    indices_.clear();
    vertexData_ = nullptr;
    indexData_ = nullptr;
}

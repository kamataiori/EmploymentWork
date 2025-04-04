#include "Light.h"
#include <numbers>
#include "MathFunctions.h"

void Light::Initialize()
{
    this->object3dCommon_ = Object3dCommon::GetInstance();

    InitializeDirectionalLight();
    InitializeCameraLight();
    InitializePointLight();
    InitializeSpotLight();
}

void Light::InitializeDirectionalLight()
{
    // 平行光源用のリソースを作成
    directionalLightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

    // データのマッピング
    directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

    // デフォルト値の設定
    directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
    directionalLightData->intensity = 1.0f;
}

void Light::InitializeCameraLight()
{
    // カメラ位置用のリソースを作成
    cameraLightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));

    // データのマッピング
    cameraLightResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraLightData));

    // デフォルト値の設定
    cameraLightData->worldPosition = { 0.0f, 0.0f, 0.0f };
}

void Light::InitializePointLight()
{
    // ポイントライト用のリソースを作成
    pointLightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(PointLight));

    // データのマッピング
    pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));

    // デフォルト値の設定
    pointLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLightData->position = { 0.0f, 2.0f, 0.0f };
    pointLightData->intensity = 0.0f;
    pointLightData->radius = 20.0f;
    pointLightData->decay = 10.0f;
}

void Light::InitializeSpotLight()
{
    // スポットライト用のリソースを作成
    spotLightResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(SpotLight));

    // データのマッピング
    spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));

    // デフォルト値の設定
    spotLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLightData->position = { 2.0f, 1.25f, 0.0f };
    spotLightData->direction = Normalize({ -1.0f, -1.0f, 0.0f });
    spotLightData->intensity = 0.0f;
    spotLightData->distance = 7.0f;
    spotLightData->decay = 2.0f;
    spotLightData->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
}

// Getter
Light::DirectionalLight* Light::GetDirectionalLight() { return directionalLightData; }
Light::CameraForGPU* Light::GetCameraLight() { return cameraLightData; }
Light::PointLight* Light::GetPointLight() { return pointLightData; }
Light::SpotLight* Light::GetSpotLight() { return spotLightData; }

// Setter
void Light::SetDirectionalLightColor(const Vector4& color) { directionalLightData->color = color; }
void Light::SetDirectionalLightDirection(const Vector3& direction) { directionalLightData->direction = Normalize(direction); }
void Light::SetDirectionalLightIntensity(float intensity) { directionalLightData->intensity = intensity; }
void Light::SetCameraPosition(const Vector3& position) { cameraLightData->worldPosition = position; }
void Light::SetPointLightColor(const Vector4& color) { pointLightData->color = color; }
void Light::SetPointLightPosition(const Vector3& position) { pointLightData->position = position; }
void Light::SetPointLightIntensity(float intensity) { pointLightData->intensity = intensity; }
void Light::SetPointLightRadius(float radius) { pointLightData->radius = radius; }
void Light::SetPointLightDecay(float decay) { pointLightData->decay = decay; }
void Light::SetSpotLightColor(const Vector4& color) { spotLightData->color = color; }
void Light::SetSpotLightPosition(const Vector3& position) { spotLightData->position = position; }
void Light::SetSpotLightDirection(const Vector3& direction) { spotLightData->direction = Normalize(direction); }
void Light::SetSpotLightIntensity(float intensity) { spotLightData->intensity = intensity; }
void Light::SetSpotLightDistance(float distance) { spotLightData->distance = distance; }
void Light::SetSpotLightDecay(float decay) { spotLightData->decay = decay; }
void Light::SetSpotLightCosAngle(float cosAngle) { spotLightData->cosAngle = cosAngle; }

// GPUアドレスの取得
D3D12_GPU_VIRTUAL_ADDRESS Light::GetDirectionalLightGPUAddress() const {
    return directionalLightResource->GetGPUVirtualAddress();
}
D3D12_GPU_VIRTUAL_ADDRESS Light::GetCameraLightGPUAddress() const {
    return cameraLightResource->GetGPUVirtualAddress();
}
D3D12_GPU_VIRTUAL_ADDRESS Light::GetPointLightGPUAddress() const {
    return pointLightResource->GetGPUVirtualAddress();
}
D3D12_GPU_VIRTUAL_ADDRESS Light::GetSpotLightGPUAddress() const {
    return spotLightResource->GetGPUVirtualAddress();
}

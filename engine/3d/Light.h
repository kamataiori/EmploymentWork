#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <numbers>
#include <wrl.h>
#include <d3d12.h>
#include "Object3dCommon.h"

class Light {
public:
    struct DirectionalLight {
        Vector4 color;  //!< ライトの色
        Vector3 direction;  //!< ライトの向き
        float intensity;  //!< 輝度
    };

    struct CameraForGPU {
        Vector3 worldPosition;
    };

    struct PointLight {
        Vector4 color;   //!< ライトの色
        Vector3 position;  //!< ライトの位置
        float intensity;  //!< 輝度
        float radius;  //! ライトの届く最大距離
        float decay;  //! 減衰率
    };

    struct SpotLight {
        Vector4 color;  //!< ライトの色
        Vector3 position;  //!< ライトの位置
        float intensity;  //!< 輝度
        Vector3 direction;  //!< スポットライトの方向
        float distance;  //!< ライトの届く最大距離
        float decay;  //! 減衰率
        float cosAngle;  //!< スポットライトの余弦
        float padding[2];  //!< パディング
    };

    DirectionalLight* directionalLightData;
    CameraForGPU* cameraLightData;
    PointLight* pointLightData;
    SpotLight* spotLightData;

    ~Light() = default;

    void Initialize();
    void InitializeDirectionalLight();
    void InitializeCameraLight();
    void InitializePointLight();
    void InitializeSpotLight();

    // Getter
    DirectionalLight* GetDirectionalLight();
    CameraForGPU* GetCameraLight();
    PointLight* GetPointLight();
    SpotLight* GetSpotLight();

    // Setter
    void SetDirectionalLightColor(const Vector4& color);
    void SetDirectionalLightDirection(const Vector3& direction);
    void SetDirectionalLightIntensity(float intensity);

    void SetCameraPosition(const Vector3& position);

    void SetPointLightColor(const Vector4& color);
    void SetPointLightPosition(const Vector3& position);
    void SetPointLightIntensity(float intensity);
    void SetPointLightRadius(float radius);
    void SetPointLightDecay(float decay);

    void SetSpotLightColor(const Vector4& color);
    void SetSpotLightPosition(const Vector3& position);
    void SetSpotLightDirection(const Vector3& direction);
    void SetSpotLightIntensity(float intensity);
    void SetSpotLightDistance(float distance);
    void SetSpotLightDecay(float decay);
    void SetSpotLightCosAngle(float cosAngle);

    // GPU バッファのアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightGPUAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetCameraLightGPUAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightGPUAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGPUAddress() const;

private:
    // Object3dCommonの初期化
    Object3dCommon* object3dCommon_ = nullptr;

    // ライト関連のバッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;   // 平行光源用
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraLightResource;        // カメラ位置用
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;         // ポイントライト用
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;          // スポットライト用
};

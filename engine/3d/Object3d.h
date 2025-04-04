#pragma once
#include "Object3dCommon.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"
#include "CameraManager.h"
#include <fstream>
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include <numbers>
#include "Light.h"
#include "Skinning.h"

//---前方宣言---//
class Object3dCommon;
class Camera;
class BaseScene;

class Object3d
{
public:

    static const int kWindowWidth = 1280;
    static const int kWindowHeight = 720;

    //--------構造体--------//

    // TransformationMatrixを拡張する
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    Object3d(BaseScene* scene);
    ~Object3d() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// ImGuiの更新
    /// </summary>
    void ImGuiUpdate(const std::string& Name);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// スケルトンの描画
    /// </summary>
    void DrawSkeleton();

    /// <summary>
    /// 座標変換行列データの初期化
    /// </summary>
    void CreateTransformationMatrixData();

    //--------Setter--------//
    // ModelのSetter
    void SetModel(const std::string& filePath) { model_ = ModelManager::GetInstance()->FindModel(filePath); }

    // Material colorのセッター
    void SetMaterialColor(const Vector4& color);

    //--------setter--------//
    void SetScale(const Vector3& scale) { this->transform.scale = scale; }
    void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
    // カメラの設定
    void SetCamera(Camera* camera) { this->camera_ = camera; } // Cameraを直接設定

    // enableLightingのセッター
    void SetEnableLighting(bool enable);


    //--------getter--------//
    const Vector3& GetScale() const { return transform.scale; }
    const Vector3& GetRotate() const { return transform.rotate; }
    const Vector3& GetTranslate() const { return transform.translate; }

    // Material colorのゲッター
    const Vector4& GetMaterialColor() const;

    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }


private:

    /// <summary>
    /// デフォルトのカメラを設定
    /// </summary>
    void SetDefaultCamera();

private:
    // Object3dCommonの初期化
    Object3dCommon* object3dCommon_ = nullptr;
    // Modelの初期化
    Model* model_ = nullptr;

    BaseScene* baseScene_ = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;  // TransformMatrix用Matrix4x4 1つ分のサイズを用意する
    TransformationMatrix* transformationMatrixData = nullptr;

    // Cameraの初期化
    Camera* camera_ = nullptr;

    Matrix4x4 worldviewProjectionMatrix;

    // Transform関連
    Transform transform;

    Matrix4x4 worldMatrix_;
};
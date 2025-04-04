#include "Object3d.h"
#include "MathFunctions.h"
#include "TextureManager.h"
#define _USE_MATH_DEFINES
#include "GamePlayScene.h"

Object3d::Object3d(BaseScene* scene)
{
    baseScene_ = scene;
}

void Object3d::Initialize()
{
    // 引数で受け取ってメンバ変数に記録する
    this->object3dCommon_ = Object3dCommon::GetInstance();

    // 座標変換行列データの初期化
    CreateTransformationMatrixData();

    // Transform変数を作る
    transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 3.14f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    // デフォルトカメラをセット
    this->camera_ = object3dCommon_->GetDefaultCamera();
}

void Object3d::Update()
{
    // model_ から modelData を取得
    const Model::ModelData& modelData = model_->GetModelData();

    if (modelData.isAnimation) {
        model_->Update();
    }

    // TransformからworldMatrixを作る
    worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // カメラTransformからカメラ行列を作る
    if (camera_) { 
        // Cameraが存在する場合
        Matrix4x4 viewProjectionMatrix = camera_->GetViewProjectionMatrix();
        worldviewProjectionMatrix = Multiply(worldMatrix_, viewProjectionMatrix);
    }
    else {
        worldviewProjectionMatrix = worldMatrix_;
    }

    /*transformationMatrixData->WVP = Multiply(modelData.rootNode.localMatrix, worldviewProjectionMatrix);*/
    transformationMatrixData->WVP = worldviewProjectionMatrix;
    transformationMatrixData->World = Multiply(modelData.rootNode.localMatrix, worldMatrix_);
}

void Object3d::ImGuiUpdate(const std::string& Name)
{
    ImGui::Begin("object3d");
    if (ImGui::TreeNode(("object3d_" + Name).c_str())) {
        ImGui::DragFloat3("translate", &transform.translate.x);
        ImGui::DragFloat3("scale", &transform.scale.x);
        ImGui::DragFloat3("rotate", &transform.rotate.x);

        ImGui::TreePop();
    }
    ImGui::End();
}

void Object3d::Draw()
{
    // 座標変換行列CBufferの場所を設定
    object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

    object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(3, baseScene_->GetLight()->GetDirectionalLightGPUAddress());

    object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(4, baseScene_->GetLight()->GetCameraLightGPUAddress());

    object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(5, baseScene_->GetLight()->GetPointLightGPUAddress());

    object3dCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(6, baseScene_->GetLight()->GetSpotLightGPUAddress());

    // 3Dモデルが割り当てられていたら描画する
    if (model_) {
        model_->Draw();
    }

    // 骨（スケルトン）を描画
    DrawSkeleton();
}

void Object3d::DrawSkeleton()
{
    if (model_) {
        model_->DrawSkeleton(worldMatrix_);
    }
}

void Object3d::CreateTransformationMatrixData()
{
    // 座標変換行列用のリソースを作成
    transformationMatrixResource = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

    // 書き込むためのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

    // 単位行列を書き込んでおく
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->World = MakeIdentity4x4();
    transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}

void Object3d::SetMaterialColor(const Vector4& color)
{
    // model_が存在する場合にのみ設定
    assert(model_);
    model_->SetMaterialColor(color);
}

void Object3d::SetEnableLighting(bool enable)
{
    assert(model_); // model_ が初期化されていることを確認
    Model::Material* material = nullptr;

    // materialData を取得
    model_->CreateMaterialData(); // 必要に応じてマテリアルデータの初期化を呼び出す
    material = model_->GetMaterial(); // Model 側で materialData を取得するゲッターを用意

    if (material) {
        material->enableLighting = enable ? 1 : 0; // bool を int32_t に変換
    }
    else {
        assert(false && "Material data is null");
    }
}

void Object3d::SetDefaultCamera()
{
    // `Object3dCommon` からデフォルトカメラを取得
    if (object3dCommon_) {
        this->camera_ = object3dCommon_->GetDefaultCamera();
    }
}

const Vector4& Object3d::GetMaterialColor() const
{
    // model_が存在する場合にのみ取得
    assert(model_);
    return model_->GetMaterialColor();
}

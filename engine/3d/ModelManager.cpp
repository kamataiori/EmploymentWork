#include "ModelManager.h"

ModelManager* ModelManager::instance = nullptr;

void ModelManager::Initialize(DirectXCommon* dxCommon)
{
    modelCommon = new ModelCommon;
    modelCommon->Initialize();
}

void ModelManager::LoadModel(const std::string& filepath)
{
    // モデルが既に読み込まれているかチェック
    if (models.contains(filepath)) {
        return; // すでに読み込まれている場合は何もしない
    }

    // モデルの生成と初期化
    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon, "Resources", filepath);

    // モデルをコンテナに格納
    models.insert(std::make_pair(filepath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    // モデルが読み込まれているかチェック
    if (models.contains(filePath)) {
        return models.at(filePath).get();
    }

    return nullptr;
}

ModelManager* ModelManager::GetInstance()
{
    if (!instance) {
        instance = new ModelManager();
    }
    return instance;
}

void ModelManager::Finalize()
{
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

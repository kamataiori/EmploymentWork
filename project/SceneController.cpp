#include "SceneController.h"

SceneController::SceneController(BaseScene* baseScene)
    : baseScene_(baseScene) {
    assert(baseScene && "SceneControllerにBaseSceneが渡っていません！");

}

void SceneController::LoadScene(const std::string& fileName)
{
    // レベルデータ読み込み
    JsonLevelLoader loader;
    levelData_.reset(loader.LevelDataLoader(fileName));

    // 既存オブジェクトクリア
    objects_.clear();

    // 全オブジェクトを生成
    for (const auto& objData : levelData_->objects) {
        // モデルの読み込み（なければロード）
        ModelManager* modelManager = ModelManager::GetInstance();
        if (!modelManager->FindModel(objData.fileName)) {
            modelManager->LoadModel(objData.fileName);
        }

        // Object3d生成
        auto obj = std::make_unique<Object3d>(baseScene_); // Sceneが必要なら引数に渡す
        obj->Initialize();
        obj->SetModel(objData.fileName);
        obj->SetTranslate(objData.translation);
        obj->SetRotate(objData.rotation);
        obj->SetScale(objData.scaling);

        objects_.push_back(std::move(obj));
    }
}

void SceneController::Update()
{
    for (auto& obj : objects_) {
        obj->Update();
    }
}

void SceneController::Draw()
{
    for (auto& obj : objects_) {
        obj->Draw();
    }
}

void SceneController::SetCamera(Camera* camera)
{
    for (auto& obj : objects_) {
        obj->SetCamera(camera);
    }
}

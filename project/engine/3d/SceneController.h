#pragma once
#include <string>
#include <memory>
#include <JsonLevelLoader.h>
#include <Object3d.h>

class SceneController {

public:
    SceneController(BaseScene* baseScene);

    // シーン読み込み
    void LoadScene(const std::string& fileName);

    // シーン更新
    void Update();

    // シーン描画
    void Draw();

    void SetCamera(Camera* camera);

private:
    // レベルデータ（JSONから読み込み）
    std::unique_ptr<LevelData> levelData_;

    // オブジェクトリスト
    std::vector<std::unique_ptr<Object3d>> objects_;

    BaseScene* baseScene_ = nullptr;
};



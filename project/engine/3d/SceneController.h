#pragma once
#include <string>
#include <memory>
#include <JsonLevelLoader.h>
#include <Object3d.h>
#include "MultiCollider.h"

class MeshBVH;

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

    // 読み込んだ全オブジェクトのメッシュからステージ用の当たり判定(BVH＋Collider)を構築する。
    // 静的ジオメトリ前提で、ワールド空間へ焼き込んで1回だけ作る。
    void BuildStageCollider();

    // ステージの当たり判定コライダー（押し戻し用・静的）。未構築なら nullptr。
    MultiCollider* GetStageCollider() const { return stageCollider_.get(); }

private:
    // レベルデータ（JSONから読み込み）
    std::unique_ptr<LevelData> levelData_;

    // オブジェクトリスト
    std::vector<std::unique_ptr<Object3d>> objects_;

    BaseScene* baseScene_ = nullptr;

    // ステージの当たり判定（メッシュBVH＋それを1つ持つCollider）
    std::shared_ptr<MeshBVH> stageBVH_;
    std::unique_ptr<MultiCollider> stageCollider_;
};



#include "SceneController.h"
#include "BVH/MeshBVH.h"
#include "Model.h"
#include <string>
#include <Windows.h>
#include "MathFunctions.h"
#include "CollisionTypeIdDef.h"

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

void SceneController::BuildStageCollider()
{
    // 全オブジェクトの三角形をワールド空間へ焼き込んで1本のリストにまとめる
    std::vector<Triangle> triangles;

    for (auto& obj : objects_) {
        Model* model = obj->GetModel();
        if (!model) continue;

        // ワールド行列を最新化してから使う
        obj->Update();

        const Model::ModelData& data = model->GetModelData();

        // 描画（Object3d::Update）は rootNode.localMatrix を掛けた行列で頂点を送っている。
        // 焼き込みも同じ行列にしないと、見えているステージと当たり判定がズレる。
        const Matrix4x4 world = Multiply(data.rootNode.localMatrix, obj->GetWorldMatrix());
        for (const auto& mesh : data.meshes) {
            const auto& verts = mesh.vertices;
            const auto& indices = mesh.indices;
            // インデックスが無いモデルは頂点をそのまま3つずつ使う
            const size_t triCount = indices.empty() ? (verts.size() / 3)
                                                    : (indices.size() / 3);
            triangles.reserve(triangles.size() + triCount);

            for (size_t i = 0; i + 2 < (indices.empty() ? verts.size() : indices.size()); i += 3) {
                const uint32_t i0 = indices.empty() ? (uint32_t)i : indices[i];
                const uint32_t i1 = indices.empty() ? (uint32_t)(i + 1) : indices[i + 1];
                const uint32_t i2 = indices.empty() ? (uint32_t)(i + 2) : indices[i + 2];

                auto toWorld = [&](const Model::VertexData& v) {
                    const Vector3 local{ v.position.x, v.position.y, v.position.z };
                    return TransformCoord(local, world);
                };

                Triangle tri{};
                tri.vertices[0] = toWorld(verts[i0]);
                tri.vertices[1] = toWorld(verts[i1]);
                tri.vertices[2] = toWorld(verts[i2]);
                tri.color = 0;
                triangles.push_back(tri);
            }
        }
    }

    // BVH構築
    stageBVH_ = std::make_shared<MeshBVH>();
    const size_t bakedTriCount = triangles.size();
    stageBVH_->Build(std::move(triangles));

    // [診断ログ] 焼き込んだ三角形数とワールド境界。
    // 三角形が0 → メッシュ未取得。境界がプレイヤーの居る範囲とズレていれば位置/スケールの問題。
    {
        const AABB rb = stageBVH_->RootBounds();
        std::string msg = "[BuildStageCollider] triangles=" + std::to_string(bakedTriCount)
            + " bounds.min=(" + std::to_string(rb.min.x) + "," + std::to_string(rb.min.y) + "," + std::to_string(rb.min.z) + ")"
            + " bounds.max=(" + std::to_string(rb.max.x) + "," + std::to_string(rb.max.y) + "," + std::to_string(rb.max.z) + ")\n";
        OutputDebugStringA(msg.c_str());
    }

    // Meshシェイプを1つ持つ静的コライダーを作る（押し戻しの受け側）
    stageCollider_ = std::make_unique<MultiCollider>();
    stageCollider_->AddMesh(stageBVH_);
    stageCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::Stage));
    stageCollider_->SetResponse(CollisionResponse::Blocking);
    stageCollider_->SetMovable(false); // ステージは動かない
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

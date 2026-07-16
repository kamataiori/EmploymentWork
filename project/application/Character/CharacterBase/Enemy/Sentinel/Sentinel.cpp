#include "Sentinel.h"
#include "Model.h"
#include "BVH/MeshBVH.h"
#include "MathFunctions.h"
#include "engine/TimeManager.h"
#include "engine/3d/Camera/Camera.h"
#include "engine/UI/IDamagePopupSink.h"

Sentinel::Sentinel(BaseScene* scene)
    : ObjectBase(scene)
{
}

void Sentinel::InitializeSentinel(const Vector3& placePos, float scale)
{
    ModelManager::GetInstance()->LoadModel(kModelName);

    object3d_->Initialize();
    object3d_->SetModel(kModelName);

    // 配置（足元を地面に合わせて少し持ち上げる）
    transform.translate = placePos;
    transform.translate.y += kGroundOffsetY;
    transform.rotate = { 0.0f, 0.0f, 0.0f };
    transform.scale = { scale, scale, scale };

    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);

    surfaceY_ = transform.translate.y; // 迫り上がりの到達点（立ち位置）
    spawnState_ = SpawnState::Active;  // 既定は「その場に立っている」
    hittable_ = true;

    hp_ = kMaxHP_;
    isDead_ = false;

    // 撃破・被弾のパーティクル
    particles_ = std::make_unique<ParticleManager>();
    particles_->Initialize(VertexDataType::Plane);
    particles_->LoadAllPresets();
    if (camera_) {
        particles_->SetCamera(camera_);
    }

    // 被弾スフィア（kSentinel＝Defaultグループ・Trigger）
    // …プレイヤーの武器が当たるとダメージ。触れてもプレイヤーは無傷。
    multiCollider_->Clear();
    Sphere sp{};
    sp.center = transform.translate + Vector3{ 0.0f, kTargetOffsetY_, 0.0f };
    sp.radius = kHitRadius;
    multiCollider_->AddSphere(sp);
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kSentinel));
    multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });

    // BVHメッシュ（押し戻し用・静的）：配置が確定したここで1回だけ焼き込む
    BuildBvhCollider();
}

void Sentinel::BuildBvhCollider()
{
    // ワールド行列を最新化してから焼き込む（スキニングは不要なので行列のみ）
    object3d_->UpdateTransform();

    Model* model = object3d_->GetModel();
    std::vector<Triangle> triangles;

    if (model) {
        const Model::ModelData& data = model->GetModelData();

        // 描画と同じ行列（rootNode.localMatrix × ワールド）で焼かないと
        // 見た目と当たり判定がズレる（SceneController と同じ手順）。
        const Matrix4x4 world = Multiply(data.rootNode.localMatrix, object3d_->GetWorldMatrix());
        for (const auto& mesh : data.meshes) {
            const auto& verts = mesh.vertices;
            const auto& indices = mesh.indices;
            const size_t indexCount = indices.empty() ? verts.size() : indices.size();
            triangles.reserve(triangles.size() + indexCount / 3);

            for (size_t i = 0; i + 2 < indexCount; i += 3) {
                const uint32_t i0 = indices.empty() ? (uint32_t)i       : indices[i];
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

    bvh_ = std::make_shared<MeshBVH>();
    bvh_->Build(std::move(triangles));

    // 押し戻しの受け側。動かないので Movable=false（MTVは相手側が全部負担）
    bvhCollider_ = std::make_unique<MultiCollider>();
    bvhCollider_->AddMesh(bvh_);
    bvhCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::Stage));
    bvhCollider_->SetResponse(CollisionResponse::Blocking);
    bvhCollider_->SetMovable(false);
}

void Sentinel::SetHiddenUnderground()
{
    // 地中へ沈めて登場を待つ。登場前は殴られても無効。
    spawnState_ = SpawnState::Hidden;
    hittable_ = false;
    transform.translate.y = surfaceY_ - kSpawnDepth_;
    object3d_->SetTranslate(transform.translate);
    object3d_->UpdateTransform();
}

void Sentinel::StartSpawn()
{
    // 地中で待機している個体だけ、迫り上がりを開始する
    if (spawnState_ == SpawnState::Hidden) {
        spawnState_ = SpawnState::Rising;
    }
}

void Sentinel::Update()
{
    if (isDead_) {
        // 破壊後もパーティクルは消えるまで進める
        if (particles_) particles_->Update();
        return;
    }

    // 迫り上がり（下から地表へ）
    if (spawnState_ == SpawnState::Rising) {
        const float dt = TimeManager::GetInstance()->GetDeltaTime();
        transform.translate.y += kSpawnRiseSpeed_ * dt;
        if (transform.translate.y >= surfaceY_) {
            transform.translate.y = surfaceY_;
            spawnState_ = SpawnState::Active;
            hittable_ = true; // 立ち切ったら殴れるようになる
        }
        object3d_->SetTranslate(transform.translate);

        // 被弾スフィアも一緒に持ち上げる
        if (!multiCollider_->GetShapes().empty()) {
            multiCollider_->MutableSphere(0).center =
                transform.translate + Vector3{ 0.0f, kTargetOffsetY_, 0.0f };
        }
    }

    // 見た目更新（EnemyCore.obj は非アニメなのでバリアの問題は起きない）
    object3d_->Update();

    if (particles_) particles_->Update();
}

void Sentinel::Draw()
{
    if (isDead_) return;
    if (spawnState_ == SpawnState::Hidden) return; // 地中待機中は描画しない
    object3d_->Draw();
}

void Sentinel::ParticleDraw()
{
    if (particles_) particles_->Draw();
}

void Sentinel::OnCollision(const CollisionInfo& info)
{
    if (isDead_) return;

    auto other = static_cast<CollisionTypeIdDef>(info.otherType);
    if (other == CollisionTypeIdDef::kPlayerWeapon ||
        other == CollisionTypeIdDef::kPlayerAttack ||
        other == CollisionTypeIdDef::PlayerBullet)
    {
        ApplyDamage(kDamagePerHit);
    }
}

void Sentinel::ApplyDamage(int amount)
{
    if (isDead_) return;
    if (!hittable_) return; // 登場前・破壊解禁前は無敵

    // 与ダメージ数値を敵の右上にポップアップ
    if (damageSink_ && amount > 0) {
        damageSink_->SpawnDamage(GetTargetCenter(), amount);
    }

    // 被弾火花
    if (particles_) {
        Transform spark{};
        spark.scale = { 1.0f, 1.0f, 1.0f };
        spark.rotate = { 0.0f, 0.0f, 0.0f };
        spark.translate = transform.translate;
        spark.translate.y += kEffectOffsetY_;
        particles_->EmitPreset(kHitSparkPreset_, spark);
    }

    hp_ -= amount;
    if (hp_ <= 0) {
        isDead_ = true;

        // 被弾スフィアを無効化（死亡後に触れて事故らないよう半径を潰す）
        if (!multiCollider_->GetShapes().empty()) {
            multiCollider_->MutableSphere(0).radius = 0.0f;
        }

        // 撃破爆発
        if (particles_) {
            Transform burst{};
            burst.scale = { 1.0f, 1.0f, 1.0f };
            burst.rotate = { 0.0f, 0.0f, 0.0f };
            burst.translate = transform.translate;
            burst.translate.y += kEffectOffsetY_;
            particles_->EmitPreset(kExplosionPreset_, burst);
        }

        // 撃破の手応え（軽いヒットストップ）
        TimeManager::GetInstance()->RequestHitStop(HitStopPreset::Heavy());
    }
}

void Sentinel::SetCamera(Camera* camera)
{
    ObjectBase::SetCamera(camera);
    if (particles_) {
        particles_->SetCamera(camera);
    }
}

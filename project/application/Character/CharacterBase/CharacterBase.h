#pragma once
#include "Transform.h"
#include "Object3d.h"
#include "Collider.h"
#include "Input.h"
#include <PostEffectManager.h>
#include <CollisionTypeIdDef.h>
#include "ParticleManager.h"
#include "ParticleEmitter.h"

class CharacterBase
{
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="baseScene"></param>
    CharacterBase(BaseScene* baseScene) : baseScene_(baseScene), collider_(nullptr) { object3d_ = std::make_unique<Object3d>(baseScene_); }

    ~CharacterBase() = default;

    /// <summary>
    /// 初期化処理
    /// </summary>
    virtual void Initialize() = 0;

    /// <summary>
    /// 更新処理
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 通常のObject専用の描画処理
    /// </summary>
    virtual void Draw() = 0;

    /// <summary>
    /// Skiningのモデル専用の描画処理
    /// </summary>
    virtual void SkinningDraw() = 0;

    /// <summary>
    /// パーティクル専用の描画処理
    /// </summary>
    virtual void ParticleDraw() = 0;

    // Sceneを取得
    BaseScene* GetBaseScene() const { return baseScene_; }

    /// Transformを取得
    const Transform& GetTransform() const { return transform; }

    void SetTarnsform(const Transform& transform_) { transform = transform_;  }

    // Transformをセット
    void SetTranslate(const Vector3& t) { transform.translate = t; }
    void SetRotate(const Vector3& t) { transform.rotate = t; }
    void SetScale(const Vector3& t) { transform.scale = t; }

    // Cameraをセット
    void SetCamera(Camera* camera) {
        camera_ = camera;
        object3d_->SetCamera(camera);
    }

    // SetColliderをセット
    void SetCollider(Collider* collider) { collider_ = collider; }

    // GetColliderをセット
    Collider* GetCollider() const { return collider_; }

protected:
    // カメラを共通保持
    Camera* camera_ = nullptr;

    // キャラクターの基本Transform
    Transform transform;

    // シーンの宣言
    BaseScene* baseScene_;

    // オブジェクトの宣言
    std::unique_ptr<Object3d> object3d_;

    // コライダーを管理
    Collider* collider_;

};

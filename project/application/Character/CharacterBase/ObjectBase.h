#pragma once
#include "Transform.h"
#include "Object3d.h"
#include "MultiCollider.h"
#include "Input.h"
#include <PostEffectManager.h>
#include <CollisionTypeIdDef.h>
#include "ParticleManager.h"
#include "ParticleEmitter.h"

class ObjectBase
{
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="baseScene"></param>
    ObjectBase(BaseScene* baseScene) : baseScene_(baseScene)
    {
        object3d_ = std::make_unique<Object3d>(baseScene_);
        multiCollider_ = std::make_unique<MultiCollider>();
    }

    ~ObjectBase() = default;

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

    /// <summary>
    /// 当たり判定の呼出し
    /// </summary>
    virtual void OnCollision() = 0;

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

    Camera* GetCamera() const { return camera_; }

    // SetColliderをセット
    //void SetCollider(MultiCollider* collider) { multiCollider_.get() = collider; }

    // GetColliderをゲット
    MultiCollider* GetMultiCollider() const { return multiCollider_.get(); }

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
    std::unique_ptr<MultiCollider> multiCollider_;

};

#pragma once
#include "Transform.h"
#include "Object3d.h"
#include "Collider.h"

class CharacterBase
{
public:

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="baseScene"></param>
    CharacterBase(BaseScene* baseScene) : baseScene_(baseScene), collider_(nullptr) { object3d_ = std::make_unique<Object3d>(baseScene_); }
    
    /// <summary>
    /// 初期化
    /// </summary>
    virtual void Initialize() = 0;

    /// <summary>
    /// 更新
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw() = 0;

    /// <summary>
    /// Transformを取得
    /// </summary>
    const Transform& GetTransform() const { return transform; }

    void SetCamera(Camera* camera) { object3d_->SetCamera(camera); }

    // `SetCollider()` を追加
    void SetCollider(Collider* collider) { collider_ = collider; }

    // `GetCollider()` も追加（必要に応じてアクセス可能）
    Collider* GetCollider() const { return collider_; }

protected:
    Transform transform; // キャラクターの基本Transform

    BaseScene* baseScene_;

    std::unique_ptr<Object3d> object3d_;

    Collider* collider_; // コライダーを管理

};

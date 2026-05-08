#pragma once
#include "Transform.h"
#include "Object3d.h"
#include "Sprite.h"
#include "MultiCollider.h"
#include "Input.h"
#include <PostEffectManager.h>
#include <CollisionTypeIdDef.h>
#include "ParticleManager.h"
#include "engine/TimeManager.h"

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

	virtual ~ObjectBase() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Initialize() = 0;

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// 背景スプライト描画処理
	/// </summary>
	virtual void BackGroundDraw() = 0;

	/// <summary>
	/// 通常のObject専用の描画処理
	/// </summary>
	virtual void Draw() = 0;

	/// <summary>
	/// 前景スプライト描画
	/// </summary>
	virtual void ForeGroundDraw() = 0;

	/// <summary>
	/// パーティクル専用の描画処理
	/// </summary>
	virtual void ParticleDraw() = 0;

	/// <summary>
	/// Skinningのモデル専用の描画処理
	/// </summary>
	virtual void AnimationDraw() = 0;

	/// <summary>
	/// 当たり判定の呼出し
	/// </summary>
	virtual void OnCollision() = 0;

	/// <summary>
	/// 当たり判定の呼出し（相手情報付き）
	/// </summary>
	virtual void OnCollision(const CollisionInfo& info)
	{
		// 互換：従来の OnCollision() を呼ぶ
		(void)info;
		OnCollision();
	}


	// Sceneを取得
	BaseScene* GetBaseScene() const { return baseScene_; }

	/// Transformを取得
	const Transform& GetTransform() const { return transform; }

	void SetTransform(const Transform& transform_) { transform = transform_; }

	// Transformをセット
	void SetTranslate(const Vector3& t) { transform.translate = t; }
	void SetRotate(const Vector3& t) { transform.rotate = t; }
	void SetScale(const Vector3& t) { transform.scale = t; }

	// Cameraをセット
	virtual void SetCamera(Camera* camera) {
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

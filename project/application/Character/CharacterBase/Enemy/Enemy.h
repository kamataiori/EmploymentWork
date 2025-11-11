#pragma once
#include "ObjectBase.h"
#include "MultiCollider.h"
#include <memory>

struct SkeltonAnimationSet {
	std::string Death = "Death";
	std::string Duck = "Duck";
	std::string HitReact = "HitReact";
	std::string Idle = "Idle";
	std::string Jump = "Jump";
	std::string Jump_Idle = "Jump_Idle";
	std::string Jump_Idlea = "Jump_Idlea";
	std::string Jump_Land = "Jump_Land";
	std::string No = "No";
	std::string Punch = "Punch";
	std::string Run = "Run";
	std::string Sword = "Sword";
	std::string Walk = "Walk";
	std::string Wave = "Wave";
	std::string Yes = "Yes";
};

class Enemy : public ObjectBase
{
public:

	Enemy(BaseScene* baseScene_) : ObjectBase(baseScene_) {}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void DrawModel();
	void SkinningDraw() override;
	void ParticleDraw() override;

	void OnCollision() override;

	Camera* GetCamera() const { return camera_; }

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

private:

	SkeltonAnimationSet animation_; // アニメーション名セット
	std::string currentAnimationName_;

	// コライダー
	//float sphereRadius_ = 2.2f;
	//Vector3 colliderOffset_ = { 0.0f, 1.8f, 0.0f }; ; // 原点からのオフセット(上方向)
	//Vector3 colliderTranslate_ = {}; // 当たり判定中心座標

	Vector3 obbSize_ = { 1.78f, 2.2f, 1.0f };
	Vector3 colliderOffset_ = { 0.0f, 1.8f, 0.0f }; // 原点(足元)→胴体中心へのオフセット
	Vector3 colliderCenter_ = {};                   // 実際のOBB中心
};


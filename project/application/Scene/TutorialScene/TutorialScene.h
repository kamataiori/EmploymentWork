#pragma once
#include "BaseScene.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include "DrawLine.h"
#include "DrawTriangle.h"
#include "Sprite.h"
#include "Fade.h"
#include "SkyBox.h"
#include <CollisionManager.h>
#include <FollowCamera.h>
#include <Player.h>
#include "application/Scene/TutorialScene/TutorialController.h"
#include <memory>

class TutorialScene : public BaseScene
{
public:
	//------メンバ関数------

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 背景描画
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景描画
	/// </summary>
	void ForeGroundDraw() override;

	/// <summary>
	/// デバッグ
	/// </summary>
	void Debug() override;

	/// <summary>
	/// 衝突判定と応答
	/// </summary>
	void CheckAllColisions();

private:

	std::unique_ptr<FollowCamera> followCamera;

	std::unique_ptr<Object3d> ground;

	std::unique_ptr<Object3d> sky;

	std::unique_ptr<Player> player_;

	std::unique_ptr<CollisionManager> collisionManager_;

	TutorialController tutorial_;
	std::unique_ptr<Sprite> guideSprite_;
	std::string currentGuidePath_;

};


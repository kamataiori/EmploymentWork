#pragma once
#include "BaseScene.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "DrawLine.h"
#include "DrawTriangle.h"
#include "Sprite.h"
#include <TexasHoldemManager.h>


// TitleScene.h の private より上などに（クラス外）
inline constexpr const char* kWindowName_ParticleControl = "Particle Control";
inline constexpr const char* kWindowName_AABBControl = "AABB Control";
inline constexpr const char* kWindowName_OBBControl = "OBB Control";
inline constexpr const char* kWindowName_SphereControl = "Sphere Control";
inline constexpr const char* kWindowName_DebugInfo = "Debug Information";
inline constexpr const char* kWindowName_TriangleControl = "Triangle Control";

class TitleScene : public BaseScene
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

	void Debug() override;

private:

	std::unique_ptr<Sprite> backGround_ = std::make_unique<Sprite>();

	TexasHoldemManager texasHoldem_;

	bool isPressed_ = false;
};


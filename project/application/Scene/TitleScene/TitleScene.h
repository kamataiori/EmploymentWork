#pragma once
#include "BaseScene.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "DrawLine.h"
#include "DrawTriangle.h"
#include "Sprite.h"
#include "SkyBox.h"

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

	// 3Dオブジェクトの初期化
	std::unique_ptr<Object3d> sneak = nullptr;
	Transform transform{};

	std::unique_ptr<Object3d> sword = nullptr;

	std::unique_ptr<Object3d> ground;
	std::unique_ptr<Object3d> sky;

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	// オービットカメラ用
	float orbitAngle_ = 0.0f;          // 現在角度（ラジアン）
	float orbitSpeed_ = 0.5f;          // 回転速度（rad/sec）
	float orbitRadius_ = 20.0f;        // 半径
	float orbitHeight_ = 3.0f;         // 高さ（Y）
	Vector3 orbitTargetOffset_ = { 0.0f, 1.0f, 0.0f }; // 注視点オフセット（頭あたり）


	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();

	std::string nextSceneName_ = "";

	std::unique_ptr<Sprite> title = std::make_unique<Sprite>();


	enum class TitlePhase {
		Idle,
		TitleCut,
	};

	TitlePhase phase_ = TitlePhase::Idle;


	// TitleCut用（上下分割）
	std::unique_ptr<Sprite> titleTop_ = nullptr;
	std::unique_ptr<Sprite> titleBottom_ = nullptr;

	// タイトル画像サイズ（画像自体が 320x180 に変更済み前提）
	Vector2 titleTexSize_ = { 320.0f, 180.0f };

	// 描画位置（画面中央よりちょい上）
	Vector2 titleCenterPos_ = { 0.0f, 0.0f };

	// 演出パラメータ
	float titleCutTimer_ = 0.0f;
	float titleCutDuration_ = 0.35f;   // 演出時間
	float titleCutDistance_ = 200.0f;  // 左右に逃がす距離（320幅ならこのくらいが丁度）
	float titleCutExtraY_ = 18.0f;     // 上下にも少し動かす（裂け感）

	bool requestedShutter_ = false;

	void StartTitleCut();
	void UpdateTitleCut(float dt);
};
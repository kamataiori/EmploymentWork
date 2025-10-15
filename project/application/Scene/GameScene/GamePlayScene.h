#pragma once
#include <Vector2.h>
#include <vector>
#include <memory>
#include <Sprite.h>
#include <Object3d.h>
#include "BaseScene.h"
#include "Audio.h"
#include "Light.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "DrawLine.h"
#include "CollisionManager.h"
#include <Enemy/Enemy.h>
#include <FollowCamera.h>
#include "SkyBox.h"

//inline constexpr const char* kWindowName_PlayerControl = "Player Control";
//inline constexpr const char* kWindowName_EnemyControl = "Enemy Control";
inline constexpr const char* kWindowName_MonsterControl = "Monster Control";

class GamePlayScene : public BaseScene
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

	/// <summary>
	/// camera1のセッター
	/// </summary>
	void SetCamera1(std::unique_ptr<Camera> newCamera)
	{
		camera1 = std::move(newCamera);
	}

	/// <summary>
	/// camera1のゲッター
	/// </summary>
	Camera* GetCamera1() const
	{
		return camera1.get();
	}

	/// <summary>
	/// 衝突判定と応答
	/// </summary>
	void CheckAllColisions();


private:

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;

	std::unique_ptr<Object3d> sky;

	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	

	std::unique_ptr<CollisionManager> collisionMAnager_;

	std::unique_ptr<SceneController> stage_;

	// ===== 逆シャッター用メンバ =====
	std::unique_ptr<Sprite> shutterTop_;
	std::unique_ptr<Sprite> shutterBottom_;

	struct ShutterOpen {
		bool   active = false; // 開演出が有効か
		float  t = 0.0f;  // 0→1 の進捗
		float  duration = 1.0f;  // 開き切るまでの秒数
		float  holdSec = 0.20f; // 開き始める前、中央で一瞬止める（黒保持）
		float  holdTimer = 0.0f;

		// 位置（Title の閉じる時と対称）:
		//   Start … 画面外（開いた最終位置）
		//   End   … 画面中央（閉じている初期位置）
		Vector2 topStart, topEnd; // 上パネル：アンカー(0.5, 1.0)
		Vector2 botStart, botEnd; // 下パネル：アンカー(0.5, 0.0)
	} shutterOpen_;

	// 開演出の開始API
	void BeginShutterOpen(float duration = 1.0f, float holdAtCenter = 0.20f);

};


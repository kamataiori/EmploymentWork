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

inline float DegToRad(float d) { return d * 3.1415926535f / 180.0f; }

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

	void ApplyCurrentCameraToAll();

private:

	// ====== フェーズ管理 ======
	enum class Phase {
		ShutterOpen,   // 1, シャッター演出で明ける（黒→開く）
		EnemyIntro,    // 3, 敵が上から降ってくる／下から撮る
		ReadyGo,       // 4, Ready→Go の表示
		Battle         // 5, フォローカメラで戦闘開始
	};
	Phase phase_ = Phase::ShutterOpen;
	float phaseTimer_ = 0.0f;

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;
	std::unique_ptr<Camera> enemyIntroCam_;  // 敵登場を下から撮る
	Camera* currentCamera_ = nullptr;  // 現在使っているカメラ

	// 敵登場カメラ調整
	struct EnemyIntroCamParams {
		Vector3 pos{ 0.0f, 0.28f, -1.55f };   // かなり低く & 近距離
		Vector3 rot{ DegToRad(-78.0f), 0.0f, 0.0f }; // 強い見上げ
		float fovY = DegToRad(65.0f);         // 少し広角
	} eicam_;


	// --- EnemyIntroの踏みつぶし演出 ---
	float stompShakeTime_ = 0.0f;   // 残りシェイク時間
	float stompShakeDur_ = 0.60f;  // シェイク総時間
	float stompShakeAmp_ = 0.82f;  // 位置シェイクの振幅（メートル想定）
	float stompFovBase_ = DegToRad(68.0f); // 基本FOV（Intro用）
	float stompFovKick_ = DegToRad(22.0f); // 着地瞬間のFOV上乗せ量


	// ===== Ready / Go =====
	std::unique_ptr<Sprite> ready_;
	std::unique_ptr<Sprite> go_;
	float readyAlpha_ = 0.0f;
	float goAlpha_ = 0.0f;

	// ===== ユーティリティ =====
	void SwitchPhase(Phase next);

	// ====== 環境 ======
	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;
	std::unique_ptr<Object3d> sky;

	// ====== 主要アクター ======
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


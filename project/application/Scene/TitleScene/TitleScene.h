#pragma once
#include "BaseScene.h"
#include "Object3d.h"
#include "ParticleManager.h"
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
	void UpdateCamera() override;
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
	// sword用ローカル調整
	Transform swordTransform{};

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


	// ============================
	// タイトル演出
	// ============================

	enum class TitlePhase {
		Idle,         // 通常：周回
		MoveToFront,  // SPACE後：正面へ回して止める
		SlashH,       // Attack02：横斬り（画面を横に切断）
		SlashD,       // Attack03：斜め斬り（左上→右下に切断）
		GlassFall,    // 斬れた画面が割れたガラスのように落下
	};

	TitlePhase phase_ = TitlePhase::Idle;

	// TitleCut用（上下分割）
	std::unique_ptr<Sprite> titleTop_ = nullptr;
	std::unique_ptr<Sprite> titleBottom_ = nullptr;

	// ============================
	// スタート操作UI（Space ／ パッドA）
	// ============================
	// Idle中だけ出す。押した瞬間に斬撃演出へ入るので、そこからは消して画に集中させる。
	std::unique_ptr<Sprite> startSpace_ = nullptr;
	std::unique_ptr<Sprite> startPadA_ = nullptr;

	float startUiCenterYRatio_ = 0.78f;  // 画面高に対する表示位置（キャラの足元より下）
	float startUiSpaceW_ = 130.0f;       // スペースキー絵の表示サイズ[px]（元絵160x64と同じ比率）
	float startUiSpaceH_ = 52.0f;
	float startUiPadSize_ = 44.0f;       // パッドA絵の表示サイズ[px]
	float startUiGap_ = 16.0f;           // 2つの絵の間隔[px]

	// タイトル画像サイズ（Initializeでメタデータから入れる）
	Vector2 titleTexSize_ = { 320.0f, 180.0f };

	// 描画位置（画面中央よりちょい上）
	Vector2 titleCenterPos_ = { 0.0f, 0.0f };

	// タイトル文字の分割スライド量
	float titleCutDistance_ = 200.0f;
	float titleCutExtraY_ = 18.0f;

	// ガラス落下開始時のタイトル文字の基準位置（ここから落下させる）
	Vector2 titleTopFallBase_ = { 0.0f, 0.0f };
	Vector2 titleBottomFallBase_ = { 0.0f, 0.0f };

	bool requestedChange_ = false;

	// ============================
	// 正面停止 & Attack待ち
	// ============================
	float orbitSpeedAbs_ = 3.0f;   // 正面へ寄せる速度（rad/sec）
	float frontStopEps_ = 0.02f;   // 止める誤差
	float desiredOrbitAngle_ = 0.0f;

	// 斬撃演出タイマー
	float slashHTimer_ = 0.0f;
	float slashHDuration_ = 0.90f; // Attack02の長さ
	float slashDTimer_ = 0.0f;
	float slashDDuration_ = 0.90f; // Attack03の長さ
	float fallTimer_ = 0.0f;
	float fallDuration_ = 1.60f;   // ガラス落下にかける時間

	// ============================
	// 内部関数
	// ============================
	void StartMoveToFront();
	void UpdateMoveToFront(float dt);

	void StartSlashH();
	void UpdateSlashH(float dt);

	void StartSlashD();
	void UpdateSlashD(float dt);

	void StartGlassFall();
	void UpdateGlassFall(float dt);

	static float NormalizeAngle(float a);
	static float DeltaAngle(float from, float to);
};

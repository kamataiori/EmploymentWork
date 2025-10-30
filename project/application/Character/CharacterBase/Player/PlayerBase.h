#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "AnimationSet.h"
#include "PlayerAnimation.h"
#include "PlayerAnimKey.h"

#include <memory>
#include <Sprite.h>

class PlayerAnimation;
enum class PlayerAnimKey : unsigned int;

class PlayerBase : public CharacterBase, public SphereCollider
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="baseScene_"></param>
	PlayerBase(BaseScene* baseScene_) : CharacterBase(baseScene_), SphereCollider(sphere) {}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	void ForeGroundDraw();

	/// <summary>
	/// 通常のObject専用の描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// Skiningのモデル専用の描画処理
	/// </summary>
	void SkinningDraw() override;

	/// <summary>
	/// パーティクル専用の描画処理
	/// </summary>
	void ParticleDraw() override;

	/// <summary>
	/// 当たり判定の呼出し
	/// </summary>
	void OnCollision() override;

	/// <summary>
	/// 全player共通の動き
	/// </summary>
	void Move();

	/// <summary>
	/// ゲームオーバーの演出
	/// </summary>
	void GameOver();

	// モデル変更（切り替え用）
	void ChangeModel(const char* modelName);

	// アニメーション解決コントローラ
	void SetAnimationController(PlayerAnimation* ctrl) { animCtrl_ = ctrl; }

	// 任意のタイミングでキー再生したいとき用（攻撃側から呼ぶ想定）
	void PlayAnimKey(PlayerAnimKey key);

protected:

	// 派生クラスで指定するモデル名
	virtual const char* GetModelName() const = 0;

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

private:

	// アニメーションの名前
	std::string currentAnimationName_;
	PlayerAnimation* animCtrl_ = nullptr;

	// 移動制御に関する構造体
	struct MoveControl {
		float speed = 0.2f;                    // 移動速度
		Vector3 direction{};                   // 入力から得た移動方向
		float dashSpeed = 1.0f;                // ダッシュ時の速度
		bool isDashing = false;                // ダッシュ中かどうか
		bool hasDashed_ = false;               // 1回だけダッシュ許可
		bool isDashKeyHeld_ = false;           // Bキーがまだ押されているかどうか
		const float kDashDuration = 0.5f;      // ダッシュ継続時間
		float dashTimer = 0.0f;                // 残りダッシュ時間
	};
	MoveControl move_;  // 移動制御

	// ジャンプに関するデータ構造体
	struct JumpControl {
		bool isJumping = false;               // ジャンプ中か
		float velocity = 0.0f;                // 上下速度
		int jumpCount = 0;                    // 現在ジャンプ回数
		const int kMaxJumpCount = 2;          // 最大ジャンプ回数
		bool canJump_ = true;                 // Spaceキーが離されたことを確認するフラグ
		const float kInitialVelocity = 0.45f; // 初速
		const float kGravity = 0.02f;         // 重力
		const float kGroundHeight = 0.0f;     // 地面高さ
	};
	// インスタンス
	JumpControl jump_;

	// 1回目だけデフォルトTransformを入れる
	bool isFirstInitialize_ = true;

	bool isGameOver = false;
	bool deathAnimLatched_ = false;

	// 死亡時ビネット演出の状態
	struct DeathVignette {
		bool   active = false;
		Vector3 color = { 0.0f, 0.0f, 0.0f };

		float  scale = 0.0f;         // 現在のスケール
		float  power = 0.0f;         // 現在の濃さ
		float  speedScale = 0.6f;    // 進行速度（Titleを参考）
		float  speedPower = 0.6f;    // 進行速度
		float  targetScale = 1.25f;  // 目標（濃くなるほど↑）
		float  targetPower = 2.2f;   // 目標（濃くなるほど↑）
	} deathVig_;

	// 開始API（内部用）
	void BeginDeathVignette(const Vector3& color = { 0,0,0 },
		float startScale = 0.0f, float startPower = 0.0f,
		float speed = 0.2f,
		float targetScale = 1.25f, float targetPower = 2.2f);

	struct DefeatOverlay {
		bool active = false;       // 演出中
		bool initialized = false;  // Sprite 初期化済みか
		float alpha = 0.0f;        // 現在アルファ
		float timer = 0.0f;        // 経過時間
		float delay = 0.2f;       // 表示開始までの遅延秒
		float fadeSec = 2.5f;      // フェードイン秒
		std::unique_ptr<Sprite> sprite;
	} defeat_;

};


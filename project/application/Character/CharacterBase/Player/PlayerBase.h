#pragma once
#include "ObjectBase.h"
#include "Collider.h"
#include "MultiCollider.h"
#include "PlayerAnimation.h"
#include "PlayerAnimKey.h"
#include "PlayerIWeapon.h"
#include <PlayerWeaponOBB.h>

class PlayerAnimation;
enum class PlayerAnimKey : unsigned int;

class PlayerBase : public ObjectBase
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="baseScene_"></param>
	PlayerBase(BaseScene* baseScene_) : ObjectBase(baseScene_) {}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 背景スプライト処理
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 通常のObject専用の描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景スプライト処理
	/// </summary>
	void ForeGroundDraw() override;

	/// <summary>
	/// Skiningのモデル専用の描画処理
	/// </summary>
	void AnimationDraw() override;

	/// <summary>
	/// パーティクル専用の描画処理
	/// </summary>
	void ParticleDraw() override;

	/// <summary>
	/// 当たり判定の呼出し
	/// </summary>
	void OnCollision() override;

	/// <summary>
	/// カメラをセット
	/// </summary>
	void SetCamera(Camera* camera) override;

	// モデル変更（切り替え用）
	void ChangeModel(const char* modelName);

	// アニメーションコントローラ
	void SetAnimationController(PlayerAnimation* ctrl) { animCtrl_ = ctrl; }

	// 任意のタイミングでキー再生したいとき用（攻撃側から呼ぶ想定）
	void PlayAnimKey(PlayerAnimKey key);

	void RequestAnimKey(PlayerAnimKey key, int priority, float lockSec = 0.0f);

	bool IsAnimLocked() const { return animLockTimer_ > 0.0f; }

	PlayerIWeapon* GetWeapon() { return weapon_.get(); }

	MultiCollider* GetWeaponCollider() {
		if (!weapon_) return nullptr;
		if (auto* w = dynamic_cast<PlayerWeaponOBB*>(weapon_.get())) {
			return w->GetMultiCollider();
		}
		return nullptr;
	}

private:

	/// <summary>
	/// 全player共通の動き
	/// </summary>
	void Move();

	/// <summary>
	/// playerのジャンプ処理
	/// </summary>
	void Jump();

	/// <summary>
	/// playerのブリンク(ダッシュ)処理
	/// </summary>
	void Blink();


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
		float speed = 0.25f;                    // 移動速度
		Vector3 direction{};                   // 入力から得た移動方向
		float dashSpeed = 1.0f;                // ダッシュ時の速度
		bool isDashing = false;                // ダッシュ中かどうか
		bool hasDashed_ = false;               // 1回だけダッシュ許可
		bool isDashKeyHeld_ = false;           // キーがまだ押されているかどうか
		const float kDashDuration = 0.2f;      // ダッシュ継続時間
		float dashTimer = 0.0f;                // 残りダッシュ時間
		Vector3 dashDir = { 0.0f, 0.0f, 0.0f };// ダッシュ開始時の向き
		const float kDashCooldown = 0.25f; // 連打防止の再装填時間
		float dashCooldown = 0.0f;
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

	std::unique_ptr<PlayerIWeapon> weapon_{};
	int currentAnimPriority_ = 0;  // 0=移動系, 10=攻撃, 20=スキル…など
	float animLockTimer_ = 0.0f;

	// コライダー
	float sphereRadius_ = 1.0f;
	Vector3 colliderOffset_ = {};   // 原点からのオフセット(上方向)
	Vector3 colliderTranslate_ = {}; // 当たり判定中心座標

	bool isCollided_ = false;  // 当たり判定フラグ

	// HP関連
	int hp_ = 10000;                     // 現在HP
	const int kMaxHP_ = 10000;           // 最大HP
	const int kDamagePerHit_ = 1;     // 1回の衝突ダメージ

	// === HPバー表示用 ===（Player用 左下）
	std::unique_ptr<Sprite> hpBarBG_;    // 背景
	std::unique_ptr<Sprite> hpBarFill_;  // 本体

	// 画面サイズ 1280x720 を想定
	float hpBarMaxWidth_ = 260.0f;      // 最大幅
	float hpBarHeight_ = 18.0f;       // 高さ
	float hpBarMarginLeft_ = 50.0f;     // 左端からのオフセット
	float hpBarMarginBottom_ = 100.0f;    // 下端からのオフセット



	/*std::unique_ptr<ParticleManager> poweder = std::make_unique<ParticleManager>();*/
};


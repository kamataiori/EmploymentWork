#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "AnimationSet.h"
#include "PlayerAnimation.h"
#include "PlayerAnimKey.h"

//class PlayerAnimation;

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

	// モデル変更（切り替え用）
	void ChangeModel(const char* modelName);

	// 派生クラスでアニメーション名を再設定する
	virtual void SetAnimationNames() = 0;

	void SetAnimationController(PlayerAnimation* ctrl) { animCtrl_ = ctrl; }

protected:

	// 派生クラスで指定するモデル名
	virtual const char* GetModelName() const = 0;

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

	/// アニメーションセットを取得する（派生クラスでオーバーライド）
	virtual const AnimationSet& GetAnimation() const = 0;

	// アニメーション解決コントローラ（Playerが用意するインスタンスを束ねる）
	PlayerAnimation* animCtrl_ = nullptr;


private:

	// アニメーションの名前
	std::string currentAnimationName_;

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

};


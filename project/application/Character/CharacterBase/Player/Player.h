#pragma once
#include "ObjectBase.h"
#include "Collider.h"
#include "MultiCollider.h"
#include "PlayerAnimation.h"
#include "PlayerAnimKey.h"
#include "PlayerIWeapon.h"
#include <PlayerWeapon.h>
#include "PlayerMover.h"
#include "Sword.h"
#include "Camera/CameraEffectController.h"

class Player : public ObjectBase
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="baseScene_"></param>
	Player(BaseScene* baseScene_) : ObjectBase(baseScene_) {}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;
	void UpdateVisual();

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
	/// Skinningのモデル専用の描画処理
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
	/// 当たり判定の呼出し（相手情報付き）
	/// 敵グループからの接触のみ被弾として扱う
	/// </summary>
	void OnCollision(const CollisionInfo& info) override;

	/// <summary>
	/// カメラをセット
	/// </summary>
	void SetCamera(Camera* camera) override;

	void SetCameraEffect(CameraEffectController* effect) { cameraEffect_ = effect; }

	CameraEffectController* GetCameraEffect() const { return cameraEffect_; }

	// 任意のタイミングでキー再生したいとき用（攻撃側から呼ぶ想定）
	void PlayAnimaKey(PlayerAnimKey key);

	// speed: アニメ再生速度の倍率（1.0=等倍 / >1で速く / <1で遅く）
	void RequestAnimaKey(PlayerAnimKey key, int priority, float lockSec = 0.0f, float speed = 1.0f);

	bool IsAnimaLocked() const { return animaLockTimer_ > 0.0f; }

	// 現在アニメの進行度 0.0〜1.0（武器側が当たり判定区間の判定に使う）
	float GetAnimationProgress() const { return object3d_->GetAnimationProgress(); }

	// 攻撃アニメ終了時に呼ぶ：優先度/ロックを解除し、移動アニメへ戻れるようにする
	void EndAttackState() { animaLockTimer_ = 0.0f; currentAnimaPriority_ = 0; }

	PlayerIWeapon* GetWeapon() { return weapon_.get(); }

	// 攻撃中（攻撃アニメ再生中）のみコライダーを返す。
	// 非攻撃時は nullptr を返し、Scene 側で登録されない＝判定が出ない。
	MultiCollider* GetWeaponCollider() {
		if (!sword_ || !sword_->IsHitEnabled()) return nullptr;
		return sword_->GetMultiCollider();
	}

	void SetAnimation(const std::string& name)
	{
		if (name.empty()) return;
		if (currentAnimationName_ == name) return;
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}

	/// <summary>
	/// 入力ロックのセット（true=演出中・入力無効 / false=通常）
	/// </summary>
	void SetInputLocked(bool locked) { inputLocked_ = locked; }

	/// <summary>
	/// 入力ロック状態を取得
	/// </summary>
	bool IsInputLocked() const { return inputLocked_; }

	bool  IsDead()         const { return isDead_; }
	float GetDeathTimer()  const { return deathTimer_; }
protected:

	// アニメーションを設定する関数
	void SetAnimationIfChanged(const std::string& name);

private:

	// アニメーションの名前
	std::string currentAnimationName_;
	PlayerAnimation animaCtrl_;

	// 移動・ジャンプ・ブリンクを担当するコンポーネント
	// （Player の transform を借りて動かす。詳細は PlayerMover.h を参照）
	std::unique_ptr<PlayerMover> mover_;

	// 1回目だけデフォルトTransformを入れる
	bool isFirstInitialize_ = true;

	std::unique_ptr<PlayerIWeapon> weapon_{};
	int currentAnimaPriority_ = 0;  // 0=移動系, 10=攻撃, 20=スキル…など
	float animaLockTimer_ = 0.0f;

	// コライダー
	float sphereRadius_ = 1.0f;
	Vector3 colliderOffset_ = {};   // 原点からのオフセット(上方向)
	Vector3 colliderTranslate_ = {}; // 当たり判定中心座標

	bool isCollided_ = false;  // 当たり判定フラグ

	// =============================
	// 演出中の入力ロックフラグ
	// =============================
	bool inputLocked_ = false;

	// HP関連
	int hp_ = 10000;                     // 現在HP
	const int kMaxHP_ = 10000;           // 最大HP
	const int kDamagePerHit_ = 10;       // 1回の衝突ダメージ

	// ===== 死亡演出 =====
	bool isDead_ = false;
	float deathTimer_ = 0.0f;
	const float kDeathDuration_ = 3.0f; // Deathアニメの長さに合わせて調整


	std::unique_ptr<Sword> sword_;

	CameraEffectController* cameraEffect_ = nullptr;

};

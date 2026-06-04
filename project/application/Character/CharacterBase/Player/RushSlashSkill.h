#pragma once
#include "IPlayerSkill.h"
#include "Vector3.h"
#include <vector>

class Player;
class ITarget;
class IEnemyTargetProvider;

//======================================================
// RushSlashSkill（突進乱舞 / アルティメット）
//------------------------------------------------------
// Q キーで発動するアルティメット。
//   プレイヤーから一番近い敵へ突進し、到達すると大ダメージ。
//   当たった敵から一番近い「まだ当てていない別の敵」へ続けて突進…を
//   生存している敵全員に1回ずつ当て終えるまで繰り返す。
//   突進到達時に直接ダメージを与えるので「絶対に当たる」。
//   発動中はプレイヤーを無敵＆入力ロックし、座標はこのスキルが駆動する。
//
// 役割分担：
//   - 突進の「振り付け」（最近傍選択・移動・ヒット間隔）はこのクラスが持つ。
//   - 敵一覧の供給は IEnemyTargetProvider（Scene が Enemy を注入）。
//   - 敵への加害は ITarget::ApplyDamage 越し（敵の具体型に依存しない）。
//   - 借り物（owner_ / provider_）は所有しない。
//======================================================
class RushSlashSkill : public IPlayerSkill {
public:

	/// <summary>
	/// 依存を注入する。owner はアニメ・座標・無敵制御に使う。
	/// </summary>
	void Initialize(Player* owner);

	/// <summary>
	/// 攻撃対象の供給元を後から差し込む（Scene 構築後に注入される）。
	/// </summary>
	void SetTargetProvider(IEnemyTargetProvider* provider) { provider_ = provider; }

	// ---- IPlayerSkill ----
	void Start() override;
	void Update(float dt) override;
	bool IsActive() const override { return active_; }

private:

	// 突進シーケンスの局面
	enum class Phase {
		Dash,    // 現在のターゲットへ突進中
		Impact,  // 命中ヒットストップ中（次の突進までの間）
	};

	/// <summary>
	/// fromPos から最も近い「生存中かつ未命中」のターゲットを返す（無ければ nullptr）。
	/// </summary>
	ITarget* FindNearestUnstruck(const Vector3& fromPos);

	/// <summary>
	/// 指定ターゲットへの突進を開始する（向き・アニメをセット）。
	/// </summary>
	void BeginDash(ITarget* target);

	/// <summary>
	/// スキルを終了し、無敵・入力ロック・アニメ優先度を解除する。
	/// </summary>
	void Finish();

private:

	// 借り物（所有しない）
	Player*               owner_    = nullptr;
	IEnemyTargetProvider* provider_ = nullptr;

	// ---- 状態 ----
	bool   active_ = false;             // 発動中か
	Phase  phase_ = Phase::Dash;        // 現在の局面
	ITarget* currentTarget_ = nullptr;  // 突進中のターゲット
	float  dashTimer_ = 0.0f;           // 現在の突進の経過[秒]（タイムアウト保険用）
	float  impactTimer_ = 0.0f;         // ヒットストップ経過[秒]
	std::vector<ITarget*> struck_;      // この発動で既に当てたターゲット（再突進しない用）
	std::vector<ITarget*> scratch_;     // 一覧取得の使い回しバッファ（毎フレ確保を避ける）

	// ---- 調整パラメータ ----
	const int   kHitDamage_     = 50;    // 1突進の大ダメージ（雑魚HP5は即死／ボスHP300を大きく削る）
	const float kDashSpeed_     = 120.0f; // 突進速度[ユニット/秒]（一気に詰め寄る）
	const float kStandoffDist_  = 2.0f;  // ターゲット中心の手前どれだけで「到達」とするか
	const float kDashTimeout_   = 1.5f;  // 1突進の上限[秒]。超えたら強制命中（無限ロック防止の保険）
	const float kImpactDuration_ = 0.10f;// 命中ごとのヒットストップ[秒]
	const int   kAnimaPriority_ = 30;    // アニメ優先度（スキル=20 より高く上書き防止）
	const float kAnimSpeed_     = 1.4f;  // 突進斬りアニメの再生速度倍率
	const float kAnimLockSec_   = 1.0f;  // 1突進あたりのアニメロック（突進中に移動アニメへ落ちない長さ）
};

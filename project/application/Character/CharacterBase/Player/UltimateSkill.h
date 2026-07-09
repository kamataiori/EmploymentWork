#pragma once
#include "IPlayerSkill.h"
#include "Vector3.h"
#include <vector>
#include <memory>
#include <array>

class Player;
class ITarget;
class IEnemyTargetProvider;
class Sprite;

//======================================================
// UltimateSkill（斬奪 / アルティメット・Qキー）
//------------------------------------------------------
// メタルギアライジングの「斬奪／ブレードモード」リスペクトの必殺技。
//   発動すると敵を含む全体がスローになり、プレイヤーから一番近い敵へ
//   ロックオン枠(2D UI)が付く。マウスの角度で画面を貫く「斬撃の線」を回し、
//   線がロックオン枠を貫通している状態で左クリックすると成功＝大ダメージ、
//   外していると失敗＝通常ダメージ。斬るたびに次に近い敵へロックオンが移り、
//   生存する敵が尽きるまで続く（各狙いに時間制限は無い＝フェーズ1）。
//
// 役割分担（RushSlashSkill と同じ流儀）：
//   - 「振り付け」（最近傍選択・狙い判定・描画）はこのクラスが持つ。
//   - 敵一覧の供給は IEnemyTargetProvider（Scene が Enemy を注入）。
//   - 敵への加害は ITarget::ApplyDamage 越し（敵の具体型に依存しない）。
//   - 借り物（owner_ / provider_）は所有しない。
//
// フェーズ1（この実装）はロックオン＋線合わせ＋斬撃のコア操作のみ。
// カメラワーク・パーティクル演出、Ultimateゲージ(%)は後続フェーズで足す。
//======================================================
class UltimateSkill : public IPlayerSkill {
public:

	UltimateSkill();
	// 前方宣言した Sprite を unique_ptr で持つため、デストラクタは cpp 側で定義する
	~UltimateSkill() override;

	/// <summary>
	/// 依存を注入し、UI用スプライトを生成する。
	/// </summary>
	void Initialize(Player* owner);

	/// <summary>
	/// 攻撃対象の供給元を後から差し込む（Scene 構築後に注入される）。
	/// </summary>
	void SetTargetProvider(IEnemyTargetProvider* provider) { provider_ = provider; }

	// ---- IPlayerSkill ----
	void Start() override;
	void Update(float dt) override;
	void Draw() override;
	bool IsActive() const override { return active_; }

private:

	// 発動中の局面
	enum class Phase {
		Aim,    // スロー中。マウスで斬撃線を合わせてクリック待ち。
		Slash,  // クリック直後。通常速度に戻して斬りモーションを再生中（終わったら再スロー）。
	};

	/// <summary>
	/// fromPos から最も近い「生存中かつ未斬撃」のターゲットを返す（無ければ nullptr）。
	/// </summary>
	ITarget* FindNearestUnstruck(const Vector3& fromPos);

	/// <summary>
	/// ワールド座標をスクリーン座標へ投影する。カメラ背後などで投影できなければ false。
	/// </summary>
	bool WorldToScreen(const Vector3& world, float& outX, float& outY) const;

	/// <summary>
	/// 斬撃線が現在のロックオン枠に合っているか。
	///   縦線なら lineX_ が枠の横範囲内、横線なら lineY_ が枠の縦範囲内にあれば成功。
	/// </summary>
	bool IsLineOnTarget() const;

	/// <summary>
	/// Aim中の自動フレーミング。カメラ(FollowCamera)の周回角を、現在の対象を
	/// 画面に収める向きへ滑らかに寄せる。対象取得直後だけ効かせ、以降はマウス微調整に任せる。
	/// </summary>
	void SteerCameraFraming(float unscaledDt);

	/// <summary>
	/// ロックオン中の対象へ滑らかに接近しつつ、対象の方を向かせる（unscaled で実時間移動）。
	/// 接近が完了したら arrived_ を true にする。
	/// </summary>
	void ApproachTarget(float unscaledDt);

	/// <summary>
	/// 見下ろし演出のカメラ姿勢を cineT_(0→1) から求める。
	///   cineT_=0: プレイヤー上空から敵もプレイヤーも見下ろす。
	///   cineT_=1: ロックオン枠（＝敵）へゆっくり寄った位置。
	/// </summary>
	void ComputeCinematicPose(Vector3& outPos, Vector3& outLook) const;

	/// <summary>
	/// スキルを終了し、スロー・無敵・入力ロック・カメラ操作を元に戻す。
	/// </summary>
	void Finish();

private:

	// 借り物（所有しない）
	Player*               owner_    = nullptr;
	IEnemyTargetProvider* provider_ = nullptr;

	// ---- 状態 ----
	bool     active_        = false;       // 発動中か
	Phase    phase_         = Phase::Aim;  // 現在の局面
	ITarget* currentTarget_ = nullptr;     // 現在ロックオン中のターゲット
	std::vector<ITarget*> struck_;     // この発動で既に斬った敵（同じ敵を二度斬らない用）
	std::vector<ITarget*> scratch_;    // 一覧取得の使い回しバッファ（毎フレ確保を避ける）

	// ---- 毎フレーム更新して Draw と共有する「狙い」の状態 ----
	bool  lineVertical_  = true;   // 今の斬撃線が縦か（true=縦/マウス左右, false=横/マウス上下）。斬るたびに交互。
	float lineX_         = 0.0f;   // 縦線のスクリーンX座標（マウスで左右に動かす）
	float lineY_         = 0.0f;   // 横線のスクリーンY座標（マウスで上下に動かす）
	float targetScreenX_ = 0.0f;   // ロックオン枠の中心スクリーン座標
	float targetScreenY_ = 0.0f;
	bool  targetOnScreen_ = false; // 対象を画面内に投影できているか
	bool  aligned_        = false; // 線が枠を貫通しているか（=このフレームにクリックすれば成功）
	float steerTimer_    = 0.0f;   // 対象取得後、自動フレーミングを効かせている経過[秒]（unscaled）
	float groundY_       = 0.0f;   // 発動時のプレイヤーの地上Y（終了時にここへ戻す）
	float lockOnTimer_   = 0.0f;   // 対象取得後、ロックオン枠が集まってくる演出の経過[秒]（unscaled）
	float commitTimer_   = 0.0f;   // 枠が集まりきってからの猶予経過[秒]。超えると Ultimate 終了。
	float blinkPhase_    = 0.0f;   // 内側四角の点滅位相（周波数を上げながら加算していく）

	// ---- 見下ろしカメラ演出（フェーズ2強化） ----
	bool    arrived_    = false;   // 対象への接近が完了したか（完了でシネマティックへ）
	bool    slowEngaged_ = false;  // 真上到着後にスローを一度だけ入れるためのフラグ
	float   approachStartDist_ = -1.0f; // この接近の開始距離（-1=未記録）。飛び込み進捗の算出に使う
	ITarget* approachSpinTarget_ = nullptr; // 飛び込み進捗を紐づけている対象（切替検知で測り直す）
	float   cineT_      = 0.0f;    // 演出の進行 0→1（上空→ロックオン枠へゆっくり寄る）
	float   cineWeight_ = 0.0f;    // 追従⇔演出のブレンド率（0=追従 / 1=演出）
	float   camHeightDrop_ = 0.0f; // 真上到着後、カメラの高さをプレイヤーの高さへ落とす進捗（0=演出高さ / 1=プレイヤー高さ）
	Vector3 cineHoldPos_{};        // 撃った後に追従へ戻す間、固定しておく演出カメラ位置
	Vector3 cineHoldLook_{};       // 同・注視点
	float   sideTarget_  = 1.0f;   // 見下ろしカメラの左右（+1=右 / -1=左）。右クリックで反転。
	float   sideCurrent_ = 1.0f;   // 実際の左右（sideTarget_ へ滑らかに寄せる）
	float   returnTimer_ = 0.0f;   // 撃った後、追従へ戻すイージングの経過[秒]
	float   returnStartWeight_ = 0.0f; // 戻し開始時のブレンド率（ここから 0 へイージング）

	// ---- 描画スプライト（白テクスチャを引き伸ばして使う） ----
	std::unique_ptr<Sprite> slashLine_;              // 画面を貫く斬撃線（1本）
	std::array<std::unique_ptr<Sprite>, 4> reticle_; // ロックオン枠（上・下・左・右の辺）
	std::array<std::unique_ptr<Sprite>, 4> innerBox_; // 収束後に内側で点滅する小さな色違い枠（四辺のみ・塗りつぶさない）

	// ---- 調整パラメータ ----
	const float kSlowTimeScale_    = 0.15f; // 発動中の全体スロー倍率（敵が遅くなる）
	const int   kSuccessDamage_    = 100;   // 枠内クリック成功時の大ダメージ
	const int   kFailDamage_       = 10;    // 枠外クリック失敗時の通常ダメージ
	const float kReticleHalfSize_  = 55.0f; // ロックオン枠の半辺長[px]（＝収束後の本来サイズ／当たり判定）
	const float kReticleThickness_ = 4.0f;  // 枠の辺の太さ[px]
	// ロックオン枠が四辺で集まってくる演出：開始時の半辺長[px]と収束時間[秒]。
	const float kReticleStartSize_ = 150.0f; // ここから kReticleHalfSize_ まで縮む
	const float kReticleConvergeSec_ = 1.6f;
	const float kLineThickness_    = 5.0f;  // 斬撃線の太さ[px]

	// 収束後の「コミット猶予」：内側で色違いの四角がだんだん速く点滅する。
	// この時間内に左クリックしなければ Ultimate 終了。
	const float kCommitWindowSec_  = 2.0f;  // 猶予時間[秒]
	const float kInnerReticleHalf_ = 28.0f; // 内側四角の半辺長[px]（枠より小さく）
	const float kBlinkFreqStart_   = 2.0f;  // 点滅の初速[Hz]
	const float kBlinkFreqEnd_     = 12.0f; // 猶予終盤の点滅速度[Hz]（だんだん速く）

	// 発動中のマウス感度倍率。スローに合わせてカメラをゆっくり動かせるよう下げる。
	const float kSlowCameraSensScale_ = 0.4f;
	// 斬撃モーションの再生設定（縦線=Attack03 / 横線=Attack01・02。毎回頭から再生する）。
	const int   kSlashAnimPriority_ = 30;   // idle/移動アニメより高く上書きさせない
	const float kSlashAnimLockSec_  = 1.0f; // 斬りモーション中に idle へ落ちない長さ[秒]
	const float kSlashAnimSpeed_    = 1.4f; // 再生速度倍率
	// 斬りモーションがこの進行度に達したら「振り終わり」とみなし、再スローして次の敵へ。
	const float kSlashEndProgress_  = 0.85f;

	// ---- カメラワーク（フェーズ2） ----
	// Aim中の自動フレーミング：対象を画面のどれだけ横へ寄せるか[rad]（0で正面。線合わせを残すため少し振る）。
	const float kFrameYawOffset_   = 0.30f;
	const float kFrameSteerSec_    = 0.35f; // 対象取得後、自動で寄せ続ける時間[秒]（以降はマウス微調整）
	const float kFrameSteerSharp_  = 20.0f; // 寄せの俊敏さ（大きいほど速く向く）

	// ロックオン中の接近：対象中心のどれだけ手前まで詰めるか[ユニット]。
	const float kApproachStandoff_ = 2.2f;
	// 接近の速さ[ユニット/秒]（unscaled＝スロー中でもプレイヤーは実時間で滑らかに詰める）。
	// Q直後は通常速度のまま一気に敵の真上へ飛び込ませるため速めにする。
	const float kApproachSpeed_    = 60.0f;
	// 対象をどれだけ上空から見下ろすか：対象中心より上にとる高さ[ユニット]。
	const float kAboveHeight_      = 3.0f;
	// 見下ろす向き(pitch)へのなじみ速さ（大きいほど素早く向く）。
	const float kLeanSharp_        = 16.0f;
	// 接近完了とみなす距離[ユニット]（これ以下で見下ろしカメラ演出へ入る）。
	const float kArriveEps_        = 0.4f;

	// ---- 見下ろしカメラ演出のパラメータ ----
	const float kCineStartHeight_  = 6.0f;  // 開始：プレイヤーからどれだけ上にカメラを置くか
	const float kCineStartRight_   = 4.0f;  // 開始：プレイヤーの右へどれだけ寄せるか[ユニット]（符号反転で左右入替）
	const float kCineStartBack_    = 4.0f;  // 開始：プレイヤーの後ろへどれだけ引くか[ユニット]（前方＝敵を見下ろす構図に）
	const float kSideSwapSharp_    = 6.0f;  // 右クリックで左右を入れ替えるときの移動の滑らかさ
	const float kCineEndHeight_    = 2.5f;  // 終了：対象中心からどれだけ上へ寄るか
	const float kCineEndBack_      = 3.0f;  // 終了：対象から水平にどれだけ引くか（真上すぎ防止）
	// 真上到着後、カメラの高さ「だけ」をプレイヤーの肩あたりへ寄せる滑らかさ（小さいほどゆっくり降りる）。
	const float kCamHeightDropSharp_ = 2.0f;
	// カメラを落とし込む先の高さ：プレイヤー原点(足元)からどれだけ上か[ユニット]。
	// MGR斬撃モードのように肩口へカメラが来るよう、肩の高さぶん上げる。
	const float kShoulderHeight_     = 1.5f;
	const float kCineDriftSpeed_   = 0.35f; // 上空→枠へ寄る速さ（cineT_/秒。小さいほどゆっくり）
	const float kCineBlendSharp_   = 12.0f; // 追従→演出へ移る滑らかさ
	const float kCineReturnDuration_ = 0.5f; // 撃った後、演出→追従へイージングで戻る時間[秒]

	// 斬った瞬間のカメラ演出（Slash中は通常速度なので等倍で効く）。
	const float kZoomPunchAmount_  = 0.12f; // 寄り幅
	const float kZoomPunchIn_      = 0.04f; // 寄る時間[秒]
	const float kZoomPunchOut_     = 0.16f; // 戻る時間[秒]
	const float kShakeDuration_    = 0.18f; // シェイク時間[秒]
	const float kShakeAmplitude_   = 0.30f; // シェイクの強さ（0.2=標準/0.4=強め）
};

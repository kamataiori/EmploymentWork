#pragma once
#include "Camera.h"
#include "engine/TweenEasing.h"

/// <summary>
/// カメラの FOV を時間経過で変化させる「ズーム演出」専用クラス
/// ・FOV を小さく → ズームイン（寄る）
/// ・FOV を大きく → ズームアウト（引く）
///
/// CameraEffectController から呼ばれる想定で作成
/// </summary>
class CameraZoom
{
public:
	/// <summary>
	/// ズーム用パラメータ
	/// 「どの FOV からどの FOV へ」「どれくらいの時間で」「どのイージングか」
	/// をひとまとめにした構造体
	/// </summary>
	struct Params
	{
		float duration = 0.25f;                        // ズームにかける時間（秒）
		float startFov = 0.45f;                        // 開始 FOV（useCurrentFov=false のときのみ使用）
		float targetFov = 0.25f;                        // 目標 FOV（小さいほどアップ）
		bool  useCurrentFov = true;                         // true の場合、開始 FOV はカメラから自動取得
		Tween::EasingFunc easing = Tween::Easing::EaseOutQuad; // 使用するイージング関数

		// ===== ビルダー風 setter =====

		/// <summary>ズームの時間（秒）を設定</summary>
		Params& Duration(float v) { duration = v; return *this; }

		/// <summary>
		/// 開始 FOV を明示的に指定する
		/// 呼び出した時点のカメラ FOV ではなく、この値から補間したいときに使う
		/// （この関数を呼ぶと自動的に useCurrentFov=false になる）
		/// </summary>
		Params& FromFov(float fov)
		{
			startFov = fov;
			useCurrentFov = false;
			return *this;
		}

		/// <summary>
		/// 目標 FOV を指定する
		/// 小さいほど「ぐっと寄る」印象になる
		/// </summary>
		Params& ToFov(float fov)
		{
			targetFov = fov;
			return *this;
		}

		/// <summary>
		/// 開始 FOV をカメラの現在値から自動取得するかどうかを指定する
		/// true にすると FromFov で指定した値は無視される
		/// </summary>
		Params& UseCurrentFov(bool use)
		{
			useCurrentFov = use;
			return *this;
		}

		/// <summary>
		/// 使用するイージング関数を指定する
		/// 例：Tween::Easing::EaseInOutCubic など
		/// </summary>
		Params& Easing(Tween::EasingFunc func)
		{
			easing = func;
			return *this;
		}
	};

	/// <summary>
	/// パンチズーム用パラメータ（溜め → 寄り → 戻り の3フェーズ）
	///
	/// ・溜め（pre）  … preZoomAmount まで preDuration かけてゆっくり寄る
	/// ・寄り（in）    … 残り（zoomAmount まで）を inDuration で一気に寄る
	/// ・戻り（out）   … 元の FOV へ outDuration で戻る
	///
	/// preDuration を 0 にすると溜めフェーズを省略し、従来どおりの2フェーズになる。
	/// </summary>
	struct PunchParams
	{
		float zoomAmount = 0.10f;    // 最終的な寄り幅（ラジアン）
		float preZoomAmount = 0.0f;     // 溜めフェーズで先に寄る量（ラジアン）
		float preDuration = 0.0f;     // 溜めフェーズの時間（秒、0で省略）
		float inDuration = 0.06f;    // 寄りフェーズの時間（秒）
		float outDuration = 0.18f;    // 戻りフェーズの時間（秒）
		Tween::EasingFunc preEasing = Tween::Easing::Linear;       // 溜めのイージング
		Tween::EasingFunc inEasing = Tween::Easing::EaseOutQuad;  // 寄りのイージング
		Tween::EasingFunc outEasing = Tween::Easing::EaseInQuad;   // 戻りのイージング
	};

public:
	/// <summary>デフォルトコンストラクタ</summary>
	CameraZoom();

	/// <summary>
	/// 毎フレームの更新処理
	/// Camera の FOV を補間しつつ、Camera::Update() を呼び、行列を更新する
	/// </summary>
	void Update(Camera* camera, float deltaTime);

	/// <summary>
	/// ズーム演出を開始する
	/// すでにズーム中の場合は、設定を上書きして再スタート
	/// </summary>
	void Start(const Params& params);

	/// <summary>
	/// シンプル版ズーム開始
	/// duration と targetFov だけ指定し、開始 FOV は「呼び出し時のカメラ FOV」を自動で使う
	/// </summary>
	void StartSimple(float duration, float targetFov,
		Tween::EasingFunc easing = Tween::Easing::EaseOutQuad);

	/// <summary>現在ズーム中かどうか</summary>
	bool IsActive() const { return active_; }

	/// <summary>ズームを強制停止する（FOV は最後に適用された値のまま）</summary>
	void Stop();

	/// <summary>
    /// パンチズーム（攻撃時の一瞬寄って戻る演出）
    /// </summary>
    /// <param name="zoomAmount">ズーム量（ラジアン、例: 0.1f）</param>
    /// <param name="inDuration">行きの時間（秒）</param>
    /// <param name="outDuration">戻りの時間（秒）</param>
	void StartPunch(float zoomAmount, float inDuration = 0.05f, float outDuration = 0.15f);

	/// <summary>
	/// パンチズーム（イージング指定版）
	/// 行き／戻りのイージングを個別に指定できる。
	/// 例：行きを EaseInExpo にすると「ゆっくり溜め → 一瞬で寄る」演出になる。
	/// </summary>
	/// <param name="zoomAmount">ズーム量（ラジアン）</param>
	/// <param name="inDuration">行きの時間（秒）</param>
	/// <param name="outDuration">戻りの時間（秒）</param>
	/// <param name="inEasing">行きのイージング関数</param>
	/// <param name="outEasing">戻りのイージング関数</param>
	void StartPunch(float zoomAmount, float inDuration, float outDuration,
		Tween::EasingFunc inEasing, Tween::EasingFunc outEasing);

	/// <summary>
	/// パンチズーム（溜め → 寄り → 戻り の3フェーズ版）
	/// 「最初はゆっくりズーム → 一気にぐんっと寄る → 戻る」のような演出に使う。
	/// </summary>
	void StartPunch(const PunchParams& params);

private:
	/// <summary>
	/// 実際の補間ロジック（内部用）
	/// </summary>
	void UpdateInternal(Camera* camera, float deltaTime);

private:
	bool  active_;          // ズーム中フラグ
	float elapsed_;         // 経過時間
	float duration_;        // 全体の時間
	float startFov_;        // 開始 FOV
	float targetFov_;       // 目標 FOV
	Tween::EasingFunc easing_; // 使用中のイージング関数

	bool  useCurrentFov_;   // Start 時に useCurrentFov を指定されたか
	bool  capturedStartFov_; // 「カメラから開始 FOV を取得済みか」のフラグ

	// ===== パンチモード（溜め → 寄り → 戻り）専用 =====
	bool  punchMode_ = false;       // パンチモード中か
	bool  punchPrePhase_ = false;   // 溜めフェーズ中か
	bool  punchReturning_ = false;  // 戻りフェーズ中か
	float punchOriginalFov_ = 0.0f; // 元のFOV（戻り先）
	float punchPreFov_ = 0.0f;      // 溜めフェーズ終了時の中間FOV
	float punchHoldFov_ = 0.0f;     // 一番ズームした時のFOV
	float punchZoomAmount_ = 0.0f;    // 最終的な寄り幅
	float punchPreZoomAmount_ = 0.0f; // 溜めフェーズの寄り幅
	float punchPreDuration_ = 0.0f; // 溜めフェーズの時間
	float punchInDuration_ = 0.0f;  // 寄りフェーズの時間
	float punchOutDuration_ = 0.0f; // 戻りフェーズの時間
	Tween::EasingFunc punchPreEasing_ = Tween::Easing::Linear;      // 溜めのイージング
	Tween::EasingFunc punchInEasing_ = Tween::Easing::EaseOutQuad;  // 寄りのイージング
	Tween::EasingFunc punchOutEasing_ = Tween::Easing::EaseInQuad;  // 戻りのイージング
};

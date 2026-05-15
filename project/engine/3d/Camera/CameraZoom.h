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

	// ===== パンチモード（行って戻る）専用 =====
	bool  punchMode_ = false;     // パンチモード中か
	float punchHoldFov_ = 0.0f;   // 一番ズームした時のFOV
	float punchOriginalFov_ = 0.0f; // 元のFOV（戻り先）
	float punchInDuration_ = 0.0f;  // 行きの時間
	float punchOutDuration_ = 0.0f; // 戻りの時間
	bool  punchReturning_ = false;  // 戻りフェーズ中か
};

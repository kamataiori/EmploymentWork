#include "CameraZoom.h"

CameraZoom::CameraZoom()
    : active_(false)
    , elapsed_(0.0f)
    , duration_(0.0f)
    , startFov_(0.0f)
    , targetFov_(0.0f)
    , easing_(nullptr)
    , useCurrentFov_(true)
    , capturedStartFov_(false)
{
}

void CameraZoom::Start(const Params& params)
{
    active_ = true;
    elapsed_ = 0.0f;
    duration_ = (params.duration > 0.0f) ? params.duration : 0.0001f;
    targetFov_ = params.targetFov;
    easing_ = params.easing;
    useCurrentFov_ = params.useCurrentFov;

    if (useCurrentFov_) {
        // 実際の開始 FOV の取得は Update() 内で、camera が渡ってきたときに行う
        capturedStartFov_ = false;
        startFov_ = params.startFov; // 仮に保持しておくが、後で上書きされる想定
    }
    else {
        // FromFov で指定された値をそのまま開始 FOV として使う
        capturedStartFov_ = true;
        startFov_ = params.startFov;
    }
}

void CameraZoom::StartSimple(float duration, float targetFov, Tween::EasingFunc easing)
{
    Params p{};
    p.duration = duration;
    p.targetFov = targetFov;
    p.useCurrentFov = true;     // 常に「今の FOV から」ズーム
    p.easing = easing;

    Start(p);
}

void CameraZoom::Stop()
{
	active_ = false;
	elapsed_ = 0.0f;
	duration_ = 0.0f;
	capturedStartFov_ = false;
	punchMode_ = false;
	punchReturning_ = false;
}

void CameraZoom::Update(Camera* camera, float deltaTime)
{
    if (!active_ || !camera) {
        return;
    }

    UpdateInternal(camera, deltaTime);
}

void CameraZoom::StartPunch(float zoomAmount, float inDuration, float outDuration)
{
	// 行きフェーズ用のパラメータをセット
	punchMode_ = true;
	punchReturning_ = false;
	punchInDuration_ = inDuration;
	punchOutDuration_ = outDuration;

	// 共通の補間状態を初期化
	active_ = true;
	elapsed_ = 0.0f;
	duration_ = inDuration;
	easing_ = Tween::Easing::EaseOutQuad;

	// 開始FOVは次のUpdateでカメラから取得
	useCurrentFov_ = true;
	capturedStartFov_ = false;

	// ターゲットは「現在FOV - zoomAmount」（後でカメラから取得して計算）
	// 一旦zoomAmountを保存しておく（targetFov_を流用）
	targetFov_ = -zoomAmount; // 負の値で「相対ズーム量」のマーカーとして使う
}

void CameraZoom::UpdateInternal(Camera* camera, float deltaTime)
{
	// ===== パンチモード専用処理 =====
	if (punchMode_) {
		// 行きフェーズ開始時：FOVを取得して目標FOVを計算
		if (!capturedStartFov_) {
			punchOriginalFov_ = camera->GetFovYRad();
			startFov_ = punchOriginalFov_;
			// targetFov_には -zoomAmount が入っているので
			float zoomAmount = -targetFov_;
			punchHoldFov_ = punchOriginalFov_ - zoomAmount;  // 寄った時のFOV
			targetFov_ = punchHoldFov_;
			capturedStartFov_ = true;
		}

		elapsed_ += deltaTime;
		float t = elapsed_ / duration_;
		if (t >= 1.0f) t = 1.0f;

		float fov = Tween::Evaluate<float>(
			startFov_,
			targetFov_,
			t,
			easing_ ? easing_ : Tween::Easing::Linear
		);

		// ラジアン→度数に変換してから渡す
		camera->SetFovYRad(fov);
		camera->Camera::Update();

		if (t >= 1.0f) {
			if (!punchReturning_) {
				// 行き終了 → 戻りフェーズへ
				punchReturning_ = true;
				startFov_ = punchHoldFov_;
				targetFov_ = punchOriginalFov_;
				duration_ = punchOutDuration_;
				elapsed_ = 0.0f;
				easing_ = Tween::Easing::EaseInQuad;
			}
			else {
				// 戻り終了 → パンチ完全終了
				punchMode_ = false;
				punchReturning_ = false;
				active_ = false;
			}
		}
		return;
	}

	// ===== 通常のズーム処理（既存のまま） =====
	if (useCurrentFov_ && !capturedStartFov_) {
		startFov_ = camera->GetFovYRad();
		capturedStartFov_ = true;
	}

	elapsed_ += deltaTime;
	float t = elapsed_ / duration_;
	if (t >= 1.0f) t = 1.0f;

	float fov = Tween::Evaluate<float>(
		startFov_,
		targetFov_,
		t,
		easing_ ? easing_ : Tween::Easing::Linear
	);

	camera->SetFovYRad(fov);
	camera->Camera::Update();

	if (t >= 1.0f) {
		active_ = false;
	}
}

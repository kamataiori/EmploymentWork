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
	punchPrePhase_ = false;
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
	// デフォルトのイージング（行き：EaseOutQuad / 戻り：EaseInQuad）で委譲する
	StartPunch(zoomAmount, inDuration, outDuration,
		Tween::Easing::EaseOutQuad, Tween::Easing::EaseInQuad);
}

void CameraZoom::StartPunch(float zoomAmount, float inDuration, float outDuration,
	Tween::EasingFunc inEasing, Tween::EasingFunc outEasing)
{
	// 溜めフェーズなし（preDuration=0）の2フェーズパンチとして PunchParams 版へ委譲
	PunchParams p{};
	p.zoomAmount = zoomAmount;
	p.preZoomAmount = 0.0f;
	p.preDuration = 0.0f;
	p.inDuration = inDuration;
	p.outDuration = outDuration;
	p.inEasing = inEasing;
	p.outEasing = outEasing;
	StartPunch(p);
}

void CameraZoom::StartPunch(const PunchParams& params)
{
	// パンチ各フェーズのパラメータを保存
	punchMode_ = true;
	punchPrePhase_ = (params.preDuration > 0.0f); // 溜め時間があれば溜めフェーズから開始
	punchReturning_ = false;

	punchZoomAmount_ = params.zoomAmount;
	punchPreZoomAmount_ = params.preZoomAmount;
	punchPreDuration_ = params.preDuration;
	punchInDuration_ = params.inDuration;
	punchOutDuration_ = params.outDuration;
	punchPreEasing_ = params.preEasing;
	punchInEasing_ = params.inEasing;
	punchOutEasing_ = params.outEasing;

	// 共通の補間状態を初期化
	// duration_ / easing_ / targetFov_ は最初の Update でフェーズに応じて設定する
	active_ = true;
	elapsed_ = 0.0f;

	// 開始FOVは次のUpdateでカメラから取得
	useCurrentFov_ = true;
	capturedStartFov_ = false;
}

void CameraZoom::UpdateInternal(Camera* camera, float deltaTime)
{
	// ===== パンチモード専用処理（溜め → 寄り → 戻り） =====
	if (punchMode_) {
		// 初回：カメラFOVを取得し、各フェーズの目標FOVと最初のフェーズを設定
		if (!capturedStartFov_) {
			punchOriginalFov_ = camera->GetFovYRad();
			punchHoldFov_ = punchOriginalFov_ - punchZoomAmount_;    // 最終の寄りFOV
			punchPreFov_ = punchOriginalFov_ - punchPreZoomAmount_; // 溜め後の中間FOV
			capturedStartFov_ = true;
			elapsed_ = 0.0f;

			if (punchPrePhase_) {
				// 溜めフェーズ：現在FOV → 中間FOV をゆっくり
				startFov_ = punchOriginalFov_;
				targetFov_ = punchPreFov_;
				duration_ = punchPreDuration_;
				easing_ = punchPreEasing_;
			}
			else {
				// 溜めなし：いきなり寄りフェーズ
				startFov_ = punchOriginalFov_;
				targetFov_ = punchHoldFov_;
				duration_ = punchInDuration_;
				easing_ = punchInEasing_;
			}
		}

		elapsed_ += deltaTime;
		float t = (duration_ > 0.0f) ? (elapsed_ / duration_) : 1.0f;
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
			if (punchPrePhase_) {
				// 溜め終了 → 寄りフェーズ（一気にぐんっと寄る）
				punchPrePhase_ = false;
				startFov_ = punchPreFov_;
				targetFov_ = punchHoldFov_;
				duration_ = punchInDuration_;
				easing_ = punchInEasing_;
				elapsed_ = 0.0f;
			}
			else if (!punchReturning_) {
				// 寄り終了 → 戻りフェーズへ
				punchReturning_ = true;
				startFov_ = punchHoldFov_;
				targetFov_ = punchOriginalFov_;
				duration_ = punchOutDuration_;
				easing_ = punchOutEasing_;
				elapsed_ = 0.0f;
			}
			else {
				// 戻り終了 → パンチ完全終了
				punchMode_ = false;
				punchPrePhase_ = false;
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

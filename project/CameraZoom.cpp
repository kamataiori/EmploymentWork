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
}

void CameraZoom::Update(Camera* camera, float deltaTime)
{
    if (!active_ || !camera) {
        return;
    }

    UpdateInternal(camera, deltaTime);
}

void CameraZoom::UpdateInternal(Camera* camera, float deltaTime)
{
    // useCurrentFov が true の場合、最初のフレームだけカメラから開始 FOV を取得する
    if (useCurrentFov_ && !capturedStartFov_) {
        startFov_ = camera->GetFovY();
        capturedStartFov_ = true;
    }

    elapsed_ += deltaTime;
    float t = elapsed_ / duration_;
    if (t >= 1.0f) {
        t = 1.0f;
    }

    // t(0～1) をイージングで変換して FOV を補間
    float fov = Tween::Evaluate<float>(
        startFov_,
        targetFov_,
        t,
        easing_ ? easing_ : Tween::Easing::Linear
    );

    camera->SetFovY(fov);
    camera->Camera::Update();  // FOV 変更を反映するために行列を再計算

    if (t >= 1.0f) {
        active_ = false;
    }
}

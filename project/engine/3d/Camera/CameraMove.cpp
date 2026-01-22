#include "CameraMove.h"
#include <cmath>

CameraMove::CameraMove()
    : active_(false)
    , elapsed_(0.0f)
    , duration_(0.0f)
    , startPos_({})
    , targetPos_({})
    , useCurrentStart_(true)
    , capturedStart_(false)
    , easing_(nullptr)
    , orbitMode_(false)
    , orbitCenter_({})
    , orbitRadius_(0.0f)
    , orbitHeight_(0.0f)
    , orbitStartAngle_(0.0f)
    , orbitEndAngle_(0.0f)
{
}

void CameraMove::Start(const Params& params)
{
    active_ = true;
    elapsed_ = 0.0f;
    duration_ = (params.duration > 0.0f) ? params.duration : 0.0001f;

    targetPos_ = params.targetPos;
    useCurrentStart_ = params.useCurrentPos;
    easing_ = params.easing;

    // 通常の直線移動なので orbit モードではない
    orbitMode_ = false;

    if (useCurrentStart_) {
        // Camera が来るまで開始位置は確定しない
        capturedStart_ = false;
        startPos_ = params.startPos; // 仮の値（あとで上書き）
    }
    else {
        // 明示された開始位置を使う
        capturedStart_ = true;
        startPos_ = params.startPos;
    }
}

void CameraMove::StartSimple(float duration, const Vector3& targetPos,
    Tween::EasingFunc easing)
{
    Params p{};
    p.duration = duration;
    p.targetPos = targetPos;
    p.useCurrentPos = true;
    p.easing = easing;
    Start(p);
}

void CameraMove::Stop()
{
    active_ = false;
    elapsed_ = 0.0f;
    duration_ = 0.0f;
    capturedStart_ = false;
    orbitMode_ = false;
}

void CameraMove::Update(Camera* camera, float deltaTime)
{
    if (!active_ || !camera) {
        return;
    }
    UpdateInternal(camera, deltaTime);
}

void CameraMove::UpdateInternal(Camera* camera, float deltaTime)
{
    // ==========================
    // 1) 回り込みモード（円運動）
    // ==========================
    if (orbitMode_) {
        elapsed_ += deltaTime;
        float t = elapsed_ / duration_;
        if (t >= 1.0f) {
            t = 1.0f;
        }

        // イージング適用
        float te = easing_ ? easing_(t) : t;

        // 角度を補間して円周上の位置を計算
        float angle = orbitStartAngle_ + (orbitEndAngle_ - orbitStartAngle_) * te;

        Vector3 newPos{};
        newPos.x = orbitCenter_.x + std::cos(angle) * orbitRadius_;
        newPos.z = orbitCenter_.z + std::sin(angle) * orbitRadius_;
        newPos.y = orbitCenter_.y + orbitHeight_;

        camera->SetTranslate(newPos);

        // 中心（player）方向を見るように Y 回転を合わせる
        Vector3 toCenter = orbitCenter_ - newPos;
        float lenXZ = std::sqrt(toCenter.x * toCenter.x + toCenter.z * toCenter.z);
        if (lenXZ > 1e-6f) {
            float yaw = std::atan2(toCenter.x, toCenter.z);

            Vector3 rot = camera->GetRotate();
            rot.y = yaw; // 左右だけ中心を見る
            // pitch も付けたければここで rot.x を計算
            camera->SetRotate(rot);
        }

        // 行列更新
        camera->Camera::Update();

        if (t >= 1.0f) {
            active_ = false;
            orbitMode_ = false;
        }
        return;
    }

    // ==========================
    // 2) 通常の直線移動
    // ==========================
    // 初回フレームで開始位置を Camera から取得
    if (useCurrentStart_ && !capturedStart_) {
        startPos_ = camera->GetTranslate();
        capturedStart_ = true;
    }

    elapsed_ += deltaTime;
    float t = elapsed_ / duration_;
    if (t >= 1.0f) {
        t = 1.0f;
    }

    // Tween（イージング適用）
    float te = easing_ ? easing_(t) : t;

    // 位置補間
    Vector3 newPos =
        Tween::Evaluate<Vector3>(startPos_, targetPos_, te, Tween::Easing::Linear);

    camera->SetTranslate(newPos);

    // Camera::Update() でビュー行列を再計算
    camera->Camera::Update();

    if (t >= 1.0f) {
        active_ = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//   撃破演出などに使える「ターゲット中心の回り込み」
///////////////////////////////////////////////////////////////////////////////
void CameraMove::StartOrbitAroundTarget(
    Camera* camera,
    const Vector3& center,
    float angleRad,
    float duration,
    Tween::EasingFunc easing)
{
    if (!camera) {
        return;
    }

    Vector3 camPos = camera->GetTranslate();

    // 中心点から見た Camera の相対位置
    Vector3 offset = camPos - center;

    // 高さ（Y）はそのまま保持
    orbitHeight_ = offset.y;

    // 半径（XZ 平面）
    offset.y = 0.0f;
    float r2 = offset.x * offset.x + offset.z * offset.z;
    if (r2 < 1e-4f) {
        // 万が一中心に近すぎる場合の保険
        offset = { 0.0f, 0.0f, -5.0f };
        r2 = 25.0f;
    }
    orbitRadius_ = std::sqrt(r2);

    // 現在角度（x,z から）
    orbitStartAngle_ = std::atan2(offset.z, offset.x);
    // 目標角度（回り込む角度を加算）
    orbitEndAngle_ = orbitStartAngle_ + angleRad;

    // 共通タイマ設定
    active_ = true;
    elapsed_ = 0.0f;
    duration_ = (duration > 0.0f) ? duration : 0.0001f;
    easing_ = easing;

    // 回り込みモードON & 中心保存
    orbitMode_ = true;
    orbitCenter_ = center;

    // 直線移動用のフラグは一応リセット
    capturedStart_ = true;
    useCurrentStart_ = false;
}

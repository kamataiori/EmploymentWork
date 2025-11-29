#include "CameraShake.h"

#include <algorithm> // std::clamp
#include <cmath>     // std::sin, std::pow
#include <random>    // std::mt19937, std::uniform_real_distribution
#include <numbers>   // std::numbers::pi_v

namespace
{
    // 2π の定数（std::numbers を使用）
    constexpr float kTwoPi = std::numbers::pi_v<float> *2.0f;

    /// <summary>
    /// 0～1 の範囲にクランプするヘルパ
    /// </summary>
    float Clamp01(float v)
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    /// <summary>
    /// 0 ～ max のランダム float を返す。シードは毎回違って OK なので簡易実装
    /// （StartShake のときにしか呼ばないのでコストは問題にならない想定）
    /// </summary>
    float RandomFloat(float max)
    {
        static std::mt19937 mt{ std::random_device{}() };
        std::uniform_real_distribution<float> dist(0.0f, max);
        return dist(mt);
    }
}

CameraShake::CameraShake()
    : active_(false)
    , elapsed_(0.0f)
    , duration_(0.0f)
    , amplitudePos_(0.0f)
    , amplitudeRot_(0.0f)
    , frequency_(10.0f)
    , damping_(1.0f)
    , affectRotation_(false)
    , mode_(ShakeMode::All)
    , seedPosX_(0.0f)
    , seedPosY_(0.0f)
    , seedPosZ_(0.0f)
    , seedRotX_(0.0f)
    , seedRotY_(0.0f)
{
}

void CameraShake::Update(Camera* camera, float deltaTime)
{
    if (!active_ || !camera) {
        return;
    }
    UpdateInternal(camera, deltaTime);
}

void CameraShake::Start(const Params& params)
{
    active_ = true;
    elapsed_ = 0.0f;
    duration_ = std::max(params.duration, 0.0001f);  // 0 に近すぎると割り算が怪しいので防止
    amplitudePos_ = params.amplitudePosition;
    amplitudeRot_ = params.amplitudeRotation;
    frequency_ = params.frequency;
    damping_ = params.damping;
    affectRotation_ = params.affectRotation;
    mode_ = params.mode;

    // 軸ごとにランダムな位相を持たせて、揺れの方向が毎回変わるようにする
    seedPosX_ = RandomFloat(kTwoPi);
    seedPosX_ = RandomFloat(kTwoPi);
    seedPosY_ = RandomFloat(kTwoPi);
    seedPosZ_ = RandomFloat(kTwoPi);
    seedRotX_ = RandomFloat(kTwoPi);
    seedRotY_ = RandomFloat(kTwoPi);
}

void CameraShake::StartSimple(float duration, float amplitudePosition, ShakeMode mode)
{
    Params p{};
    p.duration = duration;
    p.amplitudePosition = amplitudePosition;
    p.amplitudeRotation = 0.0f;
    p.frequency = 10.0f;
    p.damping = 1.0f;
    p.affectRotation = false;
    p.mode = mode;

    Start(p);
}

void CameraShake::Stop()
{
    active_ = false;
    elapsed_ = 0.0f;
    duration_ = 0.0f;
}

void CameraShake::UpdateInternal(Camera* camera, float deltaTime)
{
    elapsed_ += deltaTime;
    float t = Clamp01(elapsed_ / duration_);

    if (t >= 1.0f) {
        active_ = false;
        return;
    }

    // 減衰カーブ（1 → 0 へ向かう）
   // damping が 1.0f なら線形、2.0f なら「最初は大きく、終盤で一気に減る」感じ
    float falloff = std::pow(1.0f - t, damping_);
    float timeScaled = elapsed_ * frequency_;

    // それぞれの軸用にサイン波で揺れを生成（位相はランダム）
    float offsetX = std::sin(timeScaled + seedPosX_) * amplitudePos_ * falloff;
    float offsetY = std::sin(timeScaled * 1.37f + seedPosY_) * amplitudePos_ * falloff;
    float offsetZ = std::sin(timeScaled * 0.73f + seedPosZ_) * amplitudePos_ * falloff;

    // ==== モードに応じて揺らす軸を制限する ====
    switch (mode_) {
    case ShakeMode::All:
        break;
    case ShakeMode::Horizontal:
        offsetY = 0.0f;
        offsetZ = 0.0f;
        break;
    case ShakeMode::Vertical:
        offsetX = 0.0f;
        offsetZ = 0.0f;
        break;
    }

    // 位置
    Vector3 translate = camera->GetTranslate();
    translate.x += offsetX;
    translate.y += offsetY;
    translate.z += offsetZ;
    camera->SetTranslate(translate);

    // 回転
    if (affectRotation_ && amplitudeRot_ > 0.0f) {
        float rotPitch = std::sin(timeScaled * 1.21f + seedRotX_) * amplitudeRot_ * falloff;
        float rotYaw = std::sin(timeScaled * 0.87f + seedRotY_) * amplitudeRot_ * falloff;

        Vector3 rotate = camera->GetRotate();
        rotate.x += rotPitch;
        rotate.y += rotYaw;
        camera->SetRotate(rotate);
    }

    // ビュー行列 / ビュー射影行列を再計算（基底クラスの Update のみ呼ぶ）
    camera->Camera::Update(); // 
}

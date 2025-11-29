#pragma once
#include "Camera.h"
#include "CameraShake.h"   // シェイク専用クラス
#include "CameraZoom.h"    // ズーム専用クラス

/// <summary>
/// カメラ演出をまとめて制御する“ハブ”クラス
/// 
/// ・FollowCamera / FPSCamera / Camera など、
///   どのカメラ型にも演出を上乗せできる
/// ・実際のロジックは CameraShake / CameraZoom に分離されており、
///   このクラスはそれらをまとめて呼び出す管理役 only
/// 
/// 演出を追加したいときは、CameraZoom のような専用クラスを作って
/// ここに追加するだけで拡張できる
/// </summary>
class CameraEffectController
{
public:
    // ==== 外部に公開するエイリアス（シーン側が使うやつ） ====
    using ShakeMode = CameraShake::ShakeMode;
    using ShakeParams = CameraShake::Params;

    using ZoomParams = CameraZoom::Params;

public:
    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    CameraEffectController() = default;

    /// <summary>
    /// 毎フレームの更新処理
    /// 
    /// 【重要】
    /// 1. FollowCamera / FPSCamera の Update() を呼ぶ
    /// 2. 最後にこの Update() を呼ぶ
    /// 
    /// これで本来のカメラ挙動に演出を上乗せできる
    /// </summary>
    void Update(Camera* camera, float deltaTime);

    // ==============================
    //  シェイク API（CameraShake への委譲）
    // ==============================

    void StartShake(const ShakeParams& params) { shake_.Start(params); }

    void StartSimpleShake(float duration, float amplitudePosition, ShakeMode mode = ShakeMode::All)
    {
        shake_.StartSimple(duration, amplitudePosition, mode);
    }

    bool IsShaking() const { return shake_.IsActive(); }

    void StopShake() { shake_.Stop(); }

    // ==============================
    //  ズーム API（CameraZoom への委譲）
    // ==============================

    void StartZoom(const ZoomParams& params) { zoom_.Start(params); }

    void StartSimpleZoom(float duration, float targetFov,
        Tween::EasingFunc easing = Tween::Easing::EaseOutQuad)
    {
        zoom_.StartSimple(duration, targetFov, easing);
    }

    bool IsZooming() const { return zoom_.IsActive(); }

    void StopZoom() { zoom_.Stop(); }

private:
    // ==== 個別演出クラス ====
    CameraShake shake_;  // シェイク専用
    CameraZoom  zoom_;   // FOVズーム専用
};

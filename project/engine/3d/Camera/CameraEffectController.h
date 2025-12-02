#pragma once
#include "Camera.h"
#include "CameraShake.h"   // シェイク専用クラス
#include "CameraZoom.h"    // ズーム専用クラス
#include "CameraMove.h"

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
    using MoveParams = CameraMove::Params;

    /// <summary>
    /// 回り込み用のパラメータ構造体。
    /// シーン側はこれに center / angle / duration / easing を詰めて
    /// StartOrbitMove(camera, params) を呼ぶだけでよい。
    /// </summary>
    struct OrbitParams
    {
        Vector3 center{};            // 回り込みの中心（例：プレイヤー位置）
        float   angleRad = 0.0f;     // 回り込む角度（ラジアン）
        float   duration = 0.0f;     // 何秒かけて回り込むか
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic;

        // ===== ビルダー風 setter（チェーンで書けるように） =====

        /// <summary>回り込みの中心を設定</summary>
        OrbitParams& Center(const Vector3& c)
        {
            center = c;
            return *this;
        }

        /// <summary>回り込む角度（ラジアン）を設定</summary>
        OrbitParams& Angle(float rad)
        {
            angleRad = rad;
            return *this;
        }

        /// <summary>回り込みにかける時間（秒）を設定</summary>
        OrbitParams& Duration(float d)
        {
            duration = d;
            return *this;
        }

        /// <summary>使用するイージング関数を設定</summary>
        OrbitParams& Easing(Tween::EasingFunc func)
        {
            easing = func;
            return *this;
        }
    };

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

    /// <summary>
    /// 
    /// </summary>
    /// <param name="duration"></param>  シェイク時間
    /// <param name="amplitudePosition"></param>  位置の揺れの強さ
    /// <param name="mode"></param>
    void StartSimpleShake(float duration, float amplitudePosition, ShakeMode mode = ShakeMode::All)
    {
        shake_.StartSimple(duration, amplitudePosition, mode);
    }

    bool IsShaking() const { return shake_.IsActive(); }

    void StopShake() { shake_.Stop(); }


    // ==============================
    //  回り込み（オービット）API
    // ==============================

    /// <summary>
    /// 撃破演出などで使える「ターゲット周りの回り込み」移動
    /// camera の現在位置を起点に、center を中心に angleRad だけ回り込んだ
    /// 位置まで Tween で移動する
    /// </summary>
    void StartOrbitMove(Camera* camera,
        const Vector3& center,
        float angleRad,
        float duration,
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic)
    {
        move_.StartOrbitAroundTarget(camera, center, angleRad, duration, easing);
    }

    /// <summary>
    /// OrbitParams を使った高レベル版
    /// シーン側は OrbitParams に値を詰めて渡すだけ
    /// </summary>
    void StartOrbitMove(Camera* camera, const OrbitParams& params)
    {
        move_.StartOrbitAroundTarget(camera,
            params.center,
            params.angleRad,
            params.duration,
            params.easing);
    }


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

    // ==============================
    //  移動（CameraMove）API
    // ==============================

   /// <summary>カメラ位置移動（パラメータ指定版）</summary>
    void StartMove(const MoveParams& params) { move_.Start(params); }

    /// <summary>
    /// シンプル版移動
    /// duration と targetPos だけ指定し、開始位置は現在のカメラ位置を使う。
    /// </summary>
    void StartSimpleMove(float duration, const Vector3& targetPos,
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic)
    {
        move_.StartSimple(duration, targetPos, easing);
    }

    /// <summary>移動中かどうか</summary>
    bool IsMoving() const { return move_.IsActive(); }

    /// <summary>移動の強制停止</summary>
    void StopMove() { move_.Stop(); }

private:
    // ==== 個別演出クラス ====
    CameraShake shake_;  // シェイク専用
    CameraZoom  zoom_;   // FOVズーム専用
    CameraMove  move_;   // 位置移動用
};

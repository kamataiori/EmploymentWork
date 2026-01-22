#pragma once
#include "Camera.h"
#include "engine/TweenEasing.h"

/// <summary>
/// カメラの位置（translate）を Tween（イージング補間）で移動させる演出専用クラス
///
/// ・Start, StartSimple …… fromPos → toPos の直線移動
/// ・StartOrbitAroundTarget …… ターゲット中心の円周上で「回り込み」移動
///
/// CameraEffectController の内部で管理され、
/// GamePlayScene からは cameraEffect_->StartMove(), StartOrbitMove() を使うだけで発動できる。
/// </summary>
class CameraMove
{
public:
    /// <summary>
    /// カメラ移動のパラメータセット。
    /// 「どの位置からどこへ」「何秒かけて」「どのイージングで」
    /// をひとまとめにした構造体。
    /// </summary>
    struct Params
    {
        float duration = 0.4f;       // 移動にかける時間（秒）
        Vector3 startPos{};          // 開始位置（useCurrentPos=false のとき使用）
        Vector3 targetPos{};         // 目標位置
        bool useCurrentPos = true;   // true = Camera の現在位置を開始位置にする
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic;

        // ===== ビルダー風 setter =====

        Params& Duration(float v) { duration = v; return *this; }

        /// <summary>開始位置を明示的に指定</summary>
        Params& From(const Vector3& p)
        {
            startPos = p;
            useCurrentPos = false;
            return *this;
        }

        /// <summary>終了位置（目標位置）を指定</summary>
        Params& To(const Vector3& p)
        {
            targetPos = p;
            return *this;
        }

        /// <summary>
        /// 開始位置に「現在の Camera の位置」を使うかどうか
        /// </summary>
        Params& UseCurrentPos(bool f)
        {
            useCurrentPos = f;
            return *this;
        }

        /// <summary>イージング関数を指定</summary>
        Params& Easing(Tween::EasingFunc func)
        {
            easing = func;
            return *this;
        }
    };

public:
    CameraMove();

    /// <summary>毎フレーム更新（CameraEffectController から呼ばれる）</summary>
    void Update(Camera* camera, float deltaTime);

    /// <summary>通常の移動開始（直線補間）</summary>
    void Start(const Params& params);

    /// <summary>
    /// シンプル版移動。
    /// duration + targetPos だけ指定すれば、
    /// 「現在位置 → targetPos へ」移動する。
    /// </summary>
    void StartSimple(float duration, const Vector3& targetPos,
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic);

    /// <summary>移動中か？</summary>
    bool IsActive() const { return active_; }

    /// <summary>停止</summary>
    void Stop();

public:
    /// <summary>
    /// 撃破演出などに使える「ターゲット中心の回り込み移動」。
    ///
    /// ・Camera の現在位置を起点に  
    /// ・center を中心とした円周上で angleRad だけ角度を進めた位置をゴールとして  
    /// ・毎フレーム角度を補間して円弧を描くように移動させる。
    ///
    /// 移動中は常に center の方向へカメラの Y 回転を向ける。
    /// （※ pitch はいじらず、今の俯瞰角度を維持）
    /// </summary>
    void StartOrbitAroundTarget(
        Camera* camera,
        const Vector3& center,
        float angleRad,        // 回り込む角度（ラジアン）
        float duration,
        Tween::EasingFunc easing = Tween::Easing::EaseInOutCubic);

private:
    void UpdateInternal(Camera* camera, float deltaTime);

private:
    // ===== 共通（直線＆回り込み） =====
    bool active_ = false;
    float elapsed_ = 0.0f;
    float duration_ = 0.0f;

    Vector3 startPos_{};
    Vector3 targetPos_{};

    bool useCurrentStart_ = true;
    bool capturedStart_ = false;

    Tween::EasingFunc easing_ = nullptr;

    // ===== 回り込み専用パラメータ =====
    bool   orbitMode_ = false;  // true のときは円運動モード
    Vector3 orbitCenter_{};          // 円の中心
    float  orbitRadius_ = 0.0f;  // XZ 平面上の半径
    float  orbitHeight_ = 0.0f;  // center との高さ差
    float  orbitStartAngle_ = 0.0f;  // 開始角度
    float  orbitEndAngle_ = 0.0f;  // 終了角度
};

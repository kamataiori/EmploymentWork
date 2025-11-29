#pragma once
#include "Camera.h"

/// <summary>
/// カメラシェイク専用のクラス。
/// Camera に対して位置・回転の揺れを上乗せする責務だけを持つ
/// </summary>
class CameraShake
{
public:
    /// <summary>
    /// シェイクの方向モード
    /// </summary>
    enum class ShakeMode {
        All,        // 全方向（X,Y,Z）揺らす
        Horizontal, // 横揺れだけ（X方向）
        Vertical    // 縦揺れだけ（Y方向）
    };

    /// <summary>
    /// シェイクの設定用構造体。
    /// ビルダー風の setter を持たせて、シーン側から楽に設定できるようにしている
    /// </summary>
    struct Params
    {
        // 生の値（デフォルト値）
        float duration = 0.3f;   // シェイク時間（秒）
        float amplitudePosition = 0.2f;   // 位置の揺れの強さ
        float amplitudeRotation = 0.0f;   // 回転の揺れの強さ（ラジアン）
        float frequency = 10.0f;  // 周波数（1秒あたりの揺れ回数のイメージ）
        float damping = 1.0f;   // 減衰の強さ（1.0=線形, 2.0=最初大きく後半急に減る）
        bool  affectRotation = false;  // 回転も揺らすかどうか
        ShakeMode mode = ShakeMode::All; // シェイク方向モード

        // ===== ビルダー風 setter（チェーンで設定しやすくするための関数群） =====

        /// <summary>
        /// シェイクの継続時間（秒）を設定する
        /// 例：0.25f → 0.25 秒間揺れる
        /// </summary>
        Params& Duration(float v) {
            duration = v;
            return *this;
        }

        /// <summary>
        /// 位置の揺れ（カメラの移動量）の強さを設定する
        /// 数値が大きいほどカメラが大きく揺れる
        /// 例：0.4f → 強めの揺れ
        /// </summary>
        Params& AmpPos(float v) {
            amplitudePosition = v;
            return *this;
        }

        /// <summary>
        /// 回転の揺れの強さ（ラジアン）を設定する
        /// 0.02f など小さめが推奨。大きすぎると酔いやすくなる
        /// 例：0.02f → ちょっとだけ首振り効果
        /// </summary>
        Params& AmpRot(float v) {
            amplitudeRotation = v;
            return *this;
        }

        /// <summary>
        /// 揺れる速さ（周波数）を設定する
        /// 数字が大きいほど「ブルブル細かい揺れ」になる
        /// 例：14.0f → 細かい高速シェイク
        /// </summary>
        Params& Frequency(float v) {
            frequency = v;
            return *this;
        }

        /// <summary>
        /// 減衰（揺れが弱くなる速さ）を設定する
        /// 1.0f → 線形に減衰 / 2.0f → 初め強く後半急激に弱まる
        /// </summary>
        Params& Damping(float v) {
            damping = v;
            return *this;
        }

        /// <summary>
        /// 回転シェイクを有効にするかどうか
        /// true → 位置 + 回転の揺れ / false → 位置のみ揺らす
        /// </summary>
        Params& AffectRot(bool v) {
            affectRotation = v;
            return *this;
        }

        /// <summary>
        /// シェイク方向モードを設定する
        /// All        → X,Y,Z 全方向揺らす
        /// Horizontal → X（横揺れ）だけ揺らす
        /// Vertical   → Y（縦揺れ）だけ揺らす
        /// </summary>
        Params& Mode(ShakeMode m) {
            mode = m;
            return *this;
        }
    };

public:
    CameraShake();

    /// <summary>
    /// 毎フレームの更新処理。
    /// 渡された Camera に対して、シェイク分のオフセットを上乗せする
    /// </summary>
    void Update(Camera* camera, float deltaTime);

    /// <summary>
    /// シェイクを開始する。
    /// すでにシェイク中でも、設定を上書きして再スタートする
    /// </summary>
    void Start(const Params& params);

    /// <summary>
    /// 手軽に使えるシンプル版シェイク開始関数
    /// 時間と位置の強さとモードだけ指定して、その他はデフォルト値を使う
    /// </summary>
    void StartSimple(float duration, float amplitudePosition, ShakeMode mode = ShakeMode::All);

    /// <summary>
    /// 現在シェイク中かどうか
    /// </summary>
    bool IsActive() const { return active_; }

    /// <summary>
    /// 強制的にシェイクを終了させる
    /// </summary>
    void Stop();

private:
    void UpdateInternal(Camera* camera, float deltaTime);

private:
    // シェイク用の内部状態
    bool  active_;
    float elapsed_;
    float duration_;
    float amplitudePos_;
    float amplitudeRot_;
    float frequency_;
    float damping_;
    bool  affectRotation_;
    ShakeMode mode_;

    // 疑似ランダムな位相
    float seedPosX_;
    float seedPosY_;
    float seedPosZ_;
    float seedRotX_;
    float seedRotY_;
};

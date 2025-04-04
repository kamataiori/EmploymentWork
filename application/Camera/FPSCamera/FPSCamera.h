#pragma once
#include "Camera.h"
#include "CharacterBase.h"

class FPSCamera : public Camera
{
public:
    enum class UpdateMode
    {
        Normal,          // ノーマルなカメラ更新
        ClampedRotation, // 回転制限付きカメラ更新
        Easing,          // イージング付きカメラ更新
        Shaky,           // 手ぶれ効果付きカメラ更新
    };

    FPSCamera(CharacterBase* target, float eyeHeight);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 更新モードを設定
    /// </summary>
    void SetUpdateMode(UpdateMode mode) { updateMode = mode; }

    /// <summary>
    /// 更新モードを取得
    /// </summary>
    UpdateMode GetUpdateMode() const { return updateMode; }

public: // プレイヤー目線の高さ関係

    /// <summary>
    /// プレイヤー目線の高さを設定
    /// </summary>
    void SetEyeHeight(float height) { eyeHeight = height; }

    /// <summary>
    /// プレイヤー目線の高さを取得
    /// </summary>
    float GetEyeHeight() const { return eyeHeight; }

private:
    void UpdateNormal();
    void UpdateClampedRotation();
    void UpdateEasing();
    void UpdateShaky();

private: // メンバ変数

    CharacterBase* target; // ターゲット（プレイヤー）
    float eyeHeight;       // プレイヤーの目線の高さ（カメラの高さ）
    UpdateMode updateMode;    // 現在の更新モード
};

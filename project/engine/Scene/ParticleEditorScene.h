#pragma once
#include "BaseScene.h"
#include "ParticleManager.h"
#include "Sprite.h"
#include "Object3d.h"
#include "SkyBox.h"

inline constexpr const char* kWindowName_Particle = "Particle Control";
inline constexpr const char* kWindowName_Preset = "Particle Preset Editor";

class ParticleEditorScene : public BaseScene
{
public:
    //------メンバ関数------

    /// <summary>初期化</summary>
    void Initialize() override;

    /// <summary>終了</summary>
    void Finalize() override;

    /// <summary>更新</summary>
    void Update() override;

    /// <summary>背景描画</summary>
    void BackGroundDraw() override;

    /// <summary>描画</summary>
    void Draw() override;

    /// <summary>前景描画</summary>
    void ForeGroundDraw() override;

    /// <summary>ImGui デバッグ</summary>
    void Debug() override;

private:
    std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();          // 専用カメラ
    std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();  // プリセット対応パーティクル管理

    // Emit 位置など（ImGuiで編集＋EmitByPresetNameに渡す）
    Transform emitterTransform = {
        {1.0f, 1.0f, 1.0f},   // scale
        {0.0f, 0.0f, 0.0f},   // rotate
        {0.0f, 0.0f, 0.0f}    // translate
    };

    // Emit時に使うプリセット名（ImGuiで編集）
    std::string emitPresetName = "NewParticle";
};

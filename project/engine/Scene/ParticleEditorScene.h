#pragma once
#include "BaseScene.h"
#include "ParticleManager.h"
#include "Sprite.h"
#include "Object3d.h"
#include "SkyBox.h"

#include <vector>
#include <string>

inline constexpr const char* kWindowName_Particle = "Particle Control";
inline constexpr const char* kWindowName_Preset = "Particle Preset Editor";
inline constexpr const char* kWindowName_Console = "Console";
inline constexpr const char* kWindowName_Camera = "Camera Control";

// Niagara風 UI 用の簡易データ
struct NiagaraSystemUI {
    std::string name;
    float posX = 0.0f;
    float posY = 0.0f;
    float width = 140.0f;
    float height = 120.0f;
};

struct NiagaraEmitterUI {
    std::string name;
    std::string presetName;   // このエミッタが編集するプリセット名
    float posX = 0.0f;
    float posY = 0.0f;
    float width = 160.0f;
    float height = 260.0f;

    // 0: Name
    // 1: Emitter Settings
    // 2: Emitter Spawn
    // 3: Emitter Update
    // 4: Particle Spawn
    // 5: Particle Update
    // 6: Render
    int selectedModuleIndex = 0;
};

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

    // デバッグカメラ更新用ヘルパー
    void UpdateDebugCamera();

private:
    // ====== ヘルパー関数（Debug を分割） ======
    // 左上：現在のゲーム画面（RenderTexture）表示
    void DrawSceneViewPanel(const ImVec2& pos, const ImVec2& size, int panelFlags);
    // 中央：Niagara Canvas
    void DrawNiagaraCanvas(const ImVec2& pos, const ImVec2& size, int panelFlags);
    // 右：Emitter Inspector
    void DrawNiagaraInspector(const ImVec2& pos, const ImVec2& size, int panelFlags);
    // 下：カメラコントロール
    void DrawCameraControlPanel(const ImVec2& pos, const ImVec2& size, int panelFlags);
    // 下：カーブエディタ
    void DrawCurveEditorPanel(const ImVec2& pos, const ImVec2& size, int panelFlags);

private:
    // スカイボックス
    std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();

    // グリッド
    Plane ground;

    // カメラ
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();          // 専用カメラ
    // デバッグカメラ ON/OFF
    bool debugCameraEnabled_ = false;

    std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();  // プリセット対応パーティクル管理
    // Emit 位置など（ImGuiで編集＋EmitByPresetNameに渡す）
    Transform emitterTransform = {
        {1.0f, 1.0f, 1.0f},   // scale
        {0.0f, 0.0f, 0.0f},   // rotate
        {0.0f, 0.0f, 0.0f}    // translate
    };
    // Emit時に使うプリセット名（ImGuiで編集）
    std::string emitPresetName = "NewParticle";

    // Niagara 風 UI の System
    NiagaraSystemUI niagaraSystemUI_;

    // ===== Niagara風 UI 用データ =====
    std::vector<NiagaraSystemUI> niagaraSystems_;
    std::vector<NiagaraEmitterUI> niagaraEmitters_;
    int selectedSystemIndex_ = -1;
    int selectedEmitterIndex_ = -1;
    int systemNameCounter_ = 1;
    int emitterNameCounter_ = 1;

    // Niagara エディタ全体の表示 / 非表示
    bool showNiagaraUI_ = true;

    // エミッターのコピー用クリップボード
    NiagaraEmitterUI emitterClipboard_;
    bool hasEmitterClipboard_ = false;

    // どのカーブを下パネルで編集しているか
    enum class CurveEditorMode {
        None,
        Scale,
        Color
    };
    CurveEditorMode curveEditorMode_ = CurveEditorMode::None;
    ParticleManager::Curve1D* curveEditorTarget_ = nullptr;
    std::string curveEditorTitle_;
    
    // ==========================
    // レイアウト比率（ドラッグで変更される）
    // ==========================
    // 画面全体のうち「左（SceneView+Canvas）」と「右（Inspector）」の比率
    float ratioLeftRight_ = 0.72f; // 0.72 : 0.28 くらい（元の inspectorRatio=0.28）

    // 画面全体のうち「上（SceneView+Canvas）」と「下（Camera/Curve）」の比率
    float ratioTopBottom_ = 0.65f; // 0.65 : 0.35 くらい（元の cameraRatio=0.35）

    // 左上領域の中の「SceneView」と「Niagara Canvas」の比率
    float ratioSceneCanvas_ = 0.50f; // 0.5 : 0.5 からスタート

   // ==========================
   // System 用 UI 状態を追加
   // ==========================
   /// <summary>
   /// 新規 / 既存 System 名の入力用バッファ
   /// </summary>
    std::string systemNameInput_;

    /// <summary>
    /// 「既存 System」コンボ用インデックス
    /// （Canvas 上の selectedSystemIndex_ とは別物）
    /// </summary>
    int systemComboIndex_ = -1;

    /// <summary>
    /// 「再生する System」コンボ用インデックス
    /// </summary>
    int emitSystemComboIndex_ = -1;

};

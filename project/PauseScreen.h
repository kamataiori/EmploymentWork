#pragma once
#include "engine/UI/UIElement.h"
#include <memory>
#include <string>
#include "MathFunctions.h"
#include "engine/FrameWork/input/Input.h"
#include <engine/UI/UIManager.h>

class Sprite;

// ポーズ画面を UIElement として独立させたクラス
// GamePlayScene からは「Updateして、IsPausedならゲーム更新を止める」だけでOK
class PauseScreen final : public UIElement {
public:
    PauseScreen() = default;
    ~PauseScreen() override = default;

    // 初期化
    // titleSceneName : タイトルへ戻る時のシーン名（あなたのプロジェクトに合わせて "TITLE" 等）
    void Initialize(
        const Vector2& screenSize = { 1280.0f, 720.0f },
        const std::string& titleSceneName = "TITLE"
    );

    // UIElement
    void Update() override;
    void Draw() override;

    // GamePlayScene 側が「ゲーム更新を止める」ために参照
    bool IsPaused() const { return isPaused_; }

    void SetUIManager(UIManager* ui) { uiManager_ = ui; }

private:
    // ポーズ画面内の表示状態
    enum class PauseView {
        Menu,     // menu + ope + backTitle
        Explain,  // menu + exp
    };

    // ポーズUIの出入りアニメ状態
    enum class PauseAnimState {
        Entering,
        Idle,
        Exiting,
    };

private:
    // ポーズ開始/終了（内部でTimeScaleも制御）
    void EnterPause();
    void ExitPause();

    // ESC/escクリック共通の「戻る」挙動
    void HandlePauseBack();

    // ポーズ中マウスUI（ホバー拡大/クリック）
    void UpdatePauseMouseUI();

    // UI配置（中央位置など）
    void SetupPauseLayout();

    // アニメ制御
    void BeginPauseEnterAnim();
    void BeginPauseExitAnim();
    void UpdatePauseEnterExitAnim(float unscaledDt);

    // 当たり判定
    bool HitTestSprite(const Sprite* sp, const POINT& mouse) const;

private:

    UIManager* uiManager_ = nullptr;

    // 設定
    Vector2 screenSize_{ 1280.0f, 720.0f };
    std::string titleSceneName_ = "TITLE";

    // 状態
    bool isPaused_ = false;
    bool escLock_ = false;

    PauseView pauseView_ = PauseView::Menu;
    PauseAnimState pauseAnimState_ = PauseAnimState::Idle;

    // アニメタイム
    float pauseAnimTime_ = 0.0f;
    float pauseEnterSec_ = 0.25f;     // 1枚あたりの出る時間
    float pauseExitSec_ = 0.20f;     // 1枚あたりの戻る時間
    float pauseStaggerSec_ = 0.08f;   // 上から順に遅延

    // 目標位置（中央）と開始位置（左）
    Vector2 menuTargetPos_{};
    Vector2 opeTargetPos_{};
    Vector2 backTargetPos_{};

    Vector2 menuStartPos_{};
    Vector2 opeStartPos_{};
    Vector2 backStartPos_{};

    // サイズ
    Vector2 opeBaseSize_{ 256.0f, 64.0f };
    Vector2 backBaseSize_{ 256.0f, 64.0f };
    Vector2 hoverSize_{ 288.0f, 72.0f };

    Vector2 escBaseSize_{ 128.0f, 64.0f };
    Vector2 escHoverSize_{ 144.0f, 72.0f };

private:
    // ゲーム中のESCヒント（操作不可）
    std::unique_ptr<Sprite> escHint_;

    // ポーズUI
    std::unique_ptr<Sprite> pauseBlack_;
    std::unique_ptr<Sprite> pauseMenu_;
    std::unique_ptr<Sprite> pauseOpe_;
    std::unique_ptr<Sprite> pauseBackTitle_;
    std::unique_ptr<Sprite> pauseExp_;
    std::unique_ptr<Sprite> pauseEsc_;
};

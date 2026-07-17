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
    // titleSceneName : タイトルへ戻る時のシーン名（プロジェクトに合わせて "TITLE" 等）
    void Initialize(
        const Vector2& screenSize = { 1280.0f, 720.0f },
        const std::string& titleSceneName = "TITLE"
    );

    // UIElement
    void Update() override;
    void Draw() override;

    // ポーズ中だけモーダル扱い（ゲーム更新を止めたい）
    bool IsModal() const override { return isPaused_; }

    // 外部からポーズ状態を問い合わせる用 (シーン側がカーソル等を切り替える)
    bool IsPaused() const { return isPaused_; }

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

    // ポーズ中の項目UI（マウスのホバー/クリック と 十字キーの選択/A決定）
    void UpdatePauseMouseUI();

    // UI配置（中央位置など）
    void SetupPauseLayout();

    // アニメ制御
    void BeginPauseEnterAnima();
    void BeginPauseExitAnima();
    void UpdatePauseEnterExitAnima(float unscaledDt);

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
    PauseAnimState pauseAnimaState_ = PauseAnimState::Idle;

    // ポーズメニューの選択項目。十字キーで動かし、A で決定する。
    // マウスを項目に重ねたときもここへ同期させ、ハイライトの持ち主を1つに保つ
    //（マウスとパッドで別々に持つと、両方光るなどの食い違いが出るため）。
    static constexpr int kSelectOpe_ = 0;        // 操作説明（上）
    static constexpr int kSelectBackTitle_ = 1;  // タイトルへ戻る（下）
    int padSelection_ = kSelectOpe_;

    // アニメタイム
    float pauseAnimaTime_ = 0.0f;
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

    // 操作説明（Explain）の2枚組。縦横比を保ったまま画面へ収める。
    static constexpr float kControlsTexW_ = 600.0f;  // 画像の実寸
    static constexpr float kControlsTexH_ = 930.0f;
    static constexpr float kControlsTopMargin_ = 24.0f;
    static constexpr float kControlsBottomMargin_ = 88.0f; // 右下のescボタンを避ける
    static constexpr float kControlsGap_ = 32.0f;          // 2枚の間隔

private:
    // ゲーム中のESCヒント（操作不可）
    std::unique_ptr<Sprite> escHint_;

    // ポーズUI
    std::unique_ptr<Sprite> pauseBlack_;
    std::unique_ptr<Sprite> pauseMenu_;
    std::unique_ptr<Sprite> pauseOpe_;
    std::unique_ptr<Sprite> pauseBackTitle_;
    std::unique_ptr<Sprite> pauseEsc_;

    // 操作説明（Explain）：キーボード/マウスとコントローラーの2枚を並べて出す
    std::unique_ptr<Sprite> pauseControlsKeyboard_;
    std::unique_ptr<Sprite> pauseControlsGamepad_;
};

#include "PauseScreen.h"
#include "Sprite.h"
#include "Input.h"
#include "SceneManager.h"
#include "engine/TimeManager.h"
#include "engine/TweenEasing.h"
#include <algorithm>
#include <ChangeEffect/SceneTransitionTypes.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

void PauseScreen::Initialize(const Vector2& screenSize, const std::string& titleSceneName)
{
    screenSize_ = screenSize;
    titleSceneName_ = titleSceneName;

    // ゲーム中のESCヒント（操作不可）
    escHint_ = std::make_unique<Sprite>();
    escHint_->Initialize("Resources/escBase.png");
    escHint_->SetAnchorPoint({ 1.0f, 1.0f });
    escHint_->SetSize(escBaseSize_);
    escHint_->SetPosition({ screenSize_.x - 16.0f, screenSize_.y - 16.0f });
    escHint_->Update();

    // 半透明背景
    pauseBlack_ = std::make_unique<Sprite>();
    pauseBlack_->Initialize("Resources/Black.png");
    pauseBlack_->SetAnchorPoint({ 0.0f, 0.0f });
    pauseBlack_->SetPosition({ 0.0f, 0.0f });
    pauseBlack_->SetSize(screenSize_);
    pauseBlack_->SetColor({ 0.0f, 0.0f, 0.0f, 0.55f });
    pauseBlack_->Update();

    // メニュー画像
    pauseMenu_ = std::make_unique<Sprite>();
    pauseMenu_->Initialize("Resources/menu.png");
    pauseMenu_->SetAnchorPoint({ 0.5f, 0.5f });
    pauseMenu_->SetSize({ 256.0f, 128.0f });

    pauseOpe_ = std::make_unique<Sprite>();
    pauseOpe_->Initialize("Resources/ope.png");
    pauseOpe_->SetAnchorPoint({ 0.5f, 0.5f });
    pauseOpe_->SetSize(opeBaseSize_);

    pauseBackTitle_ = std::make_unique<Sprite>();
    pauseBackTitle_->Initialize("Resources/backTitle.png");
    pauseBackTitle_->SetAnchorPoint({ 0.5f, 0.5f });
    pauseBackTitle_->SetSize(backBaseSize_);

    pauseExp_ = std::make_unique<Sprite>();
    pauseExp_->Initialize("Resources/exp.png");
    pauseExp_->SetAnchorPoint({ 0.5f, 0.5f });
    pauseExp_->SetSize({ 256.0f, 64.0f });

    // ポーズ中のESC（操作可能）
    pauseEsc_ = std::make_unique<Sprite>();
    pauseEsc_->Initialize("Resources/esc.png");
    pauseEsc_->SetAnchorPoint({ 1.0f, 1.0f });
    pauseEsc_->SetSize(escBaseSize_);
    pauseEsc_->SetPosition({ screenSize_.x - 16.0f, screenSize_.y - 16.0f });
    pauseEsc_->Update();

    // 目標位置を作る
    SetupPauseLayout();

    // UIElementとして最前面
    SetLayer(100000);
}

void PauseScreen::Update()
{
    Input* input = Input::GetInstance();

    // ESC長押しで点滅しないロック方式
    if (input->PushKey(DIK_ESCAPE)) {
        if (!escLock_) {
            escLock_ = true;

            if (!isPaused_) {
                EnterPause();
            }
            else {
                // ポーズ中は「戻る」挙動（説明中ならメニューへ、メニューなら退出アニメ）
                HandlePauseBack();
            }
        }
    }
    else {
        escLock_ = false;
    }

    // ポーズ中だけUI操作・アニメ
    if (!isPaused_) return;

    const float udt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

    // Enter/Exit中はアニメのみ（クリック受付しない方が事故が少ない）
    if (pauseAnimState_ == PauseAnimState::Entering || pauseAnimState_ == PauseAnimState::Exiting) {
        UpdatePauseEnterExitAnim(udt);
        return;
    }

    // Idle のときだけ操作受付
    UpdatePauseMouseUI();
}

void PauseScreen::Draw()
{
    // ゲーム中は右下に escBase.png（操作不可）
    if (!isPaused_) {
        if (escHint_) escHint_->Draw();
        return;
    }

    // ポーズ中描画
    pauseBlack_->Draw();
    pauseMenu_->Draw();

    if (pauseView_ == PauseView::Menu) {
        pauseOpe_->Draw();
        pauseBackTitle_->Draw();
    }
    else {
        pauseExp_->Draw();
    }

    // どの画面でも右下ESC
    pauseEsc_->Draw();
}

void PauseScreen::EnterPause()
{
    if (isPaused_) return;
    isPaused_ = true;

    TimeManager::GetInstance()->SetTimeScale(0.0f);
    if (uiManager_) {
        uiManager_->SetPaused(true);
    }

    BeginPauseEnterAnim();
}

void PauseScreen::ExitPause()
{
    if (!isPaused_) return;
    isPaused_ = false;

    TimeManager::GetInstance()->SetTimeScale(1.0f);
    if (uiManager_) {
        uiManager_->SetPaused(false);
    }
}

void PauseScreen::HandlePauseBack()
{
    if (!isPaused_) return;

    if (pauseView_ == PauseView::Explain) {
        // 操作説明中 -> ポーズメニューに戻る
        pauseView_ = PauseView::Menu;
        return;
    }

    // ポーズメニュー中 -> 退出アニメ
    BeginPauseExitAnim();
}

void PauseScreen::SetupPauseLayout()
{
    const float centerX = screenSize_.x * 0.5f;
    const float menuY = 220.0f;

    pauseMenu_->SetPosition({ centerX, menuY });

    const float opeY = menuY + 64.0f + 24.0f + 32.0f;
    pauseOpe_->SetPosition({ centerX, opeY });

    const float backY = opeY + 32.0f + 16.0f + 32.0f;
    pauseBackTitle_->SetPosition({ centerX, backY });

    // expはope位置に出す
    pauseExp_->SetPosition({ centerX, opeY });

    pauseMenu_->Update();
    pauseOpe_->Update();
    pauseBackTitle_->Update();
    pauseExp_->Update();

    // 目標位置（中央）を保存
    menuTargetPos_ = pauseMenu_->GetPosition();
    opeTargetPos_ = pauseOpe_->GetPosition();
    backTargetPos_ = pauseBackTitle_->GetPosition();

    // 開始位置（左から中央へ）
    menuStartPos_ = { -256.0f, menuTargetPos_.y };
    opeStartPos_ = { -256.0f, opeTargetPos_.y };
    backStartPos_ = { -256.0f, backTargetPos_.y };
}

void PauseScreen::BeginPauseEnterAnim()
{
    pauseAnimState_ = PauseAnimState::Entering;
    pauseAnimTime_ = 0.0f;

    // 左からスタート
    pauseMenu_->SetPosition(menuStartPos_);
    pauseOpe_->SetPosition(opeStartPos_);
    pauseBackTitle_->SetPosition(backStartPos_);

    pauseMenu_->Update();
    pauseOpe_->Update();
    pauseBackTitle_->Update();
}

void PauseScreen::BeginPauseExitAnim()
{
    if (pauseAnimState_ == PauseAnimState::Exiting) return;

    pauseAnimState_ = PauseAnimState::Exiting;
    pauseAnimTime_ = 0.0f;
}

void PauseScreen::UpdatePauseEnterExitAnim(float unscaledDt)
{
    pauseAnimTime_ += unscaledDt;

    const bool entering = (pauseAnimState_ == PauseAnimState::Entering);
    const float dur = entering ? pauseEnterSec_ : pauseExitSec_;

    const float tMenuStart = 0.0f;
    const float tOpeStart = pauseStaggerSec_;
    const float tBackStart = pauseStaggerSec_ * 2.0f;

    auto lerp = [](const Vector2& a, const Vector2& b, float t) -> Vector2 {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
        };

    auto localT = [&](float start) -> float {
        const float x = (pauseAnimTime_ - start) / std::max(dur, 0.0001f);
        return std::clamp(x, 0.0f, 1.0f);
        };

    // menu
    {
        float t = Tween::Easing::EaseInOutCubic(localT(tMenuStart));
        const Vector2 a = entering ? menuStartPos_ : menuTargetPos_;
        const Vector2 b = entering ? menuTargetPos_ : menuStartPos_;
        pauseMenu_->SetPosition(lerp(a, b, t));
    }
    // ope
    {
        float t = Tween::Easing::EaseInOutCubic(localT(tOpeStart));
        const Vector2 a = entering ? opeStartPos_ : opeTargetPos_;
        const Vector2 b = entering ? opeTargetPos_ : opeStartPos_;
        pauseOpe_->SetPosition(lerp(a, b, t));
    }
    // backTitle
    {
        float t = Tween::Easing::EaseInOutCubic(localT(tBackStart));
        const Vector2 a = entering ? backStartPos_ : backTargetPos_;
        const Vector2 b = entering ? backTargetPos_ : backStartPos_;
        pauseBackTitle_->SetPosition(lerp(a, b, t));
    }

    pauseMenu_->Update();
    pauseOpe_->Update();
    pauseBackTitle_->Update();

    // 完了判定（最後の要素が終わる時間）
    const float endTime = tBackStart + dur;

    if (pauseAnimTime_ >= endTime) {
        if (pauseAnimState_ == PauseAnimState::Entering) {
            pauseAnimState_ = PauseAnimState::Idle;
        }
        else if (pauseAnimState_ == PauseAnimState::Exiting) {
            pauseAnimState_ = PauseAnimState::Idle;
            ExitPause(); // アニメ終わってから再開
        }
    }
}

bool PauseScreen::HitTestSprite(const Sprite* sp, const POINT& mouse) const
{
    if (!sp) return false;

    const Vector2 pos = sp->GetPosition();
    const Vector2 size = sp->GetSize();
    const Vector2 anchor = sp->GetAnchorPoint();

    const float left = pos.x - size.x * anchor.x;
    const float top = pos.y - size.y * anchor.y;
    const float right = left + size.x;
    const float bottom = top + size.y;

    const float mx = static_cast<float>(mouse.x);
    const float my = static_cast<float>(mouse.y);

    return (mx >= left && mx <= right && my >= top && my <= bottom);
}

void PauseScreen::UpdatePauseMouseUI()
{
    Input* input = Input::GetInstance();
    const POINT m = input->GetMousePosition();

    // esc（どの画面でも有効）
    const bool hoverEsc = HitTestSprite(pauseEsc_.get(), m);
    pauseEsc_->SetSize(hoverEsc ? escHoverSize_ : escBaseSize_);
    pauseEsc_->Update();

    if (hoverEsc && input->TriggerMouseButton(0)) {
        HandlePauseBack();
        return;
    }

    // Menu中のみ ope/backTitle を操作
    if (pauseView_ == PauseView::Menu) {

        const bool hoverOpe = HitTestSprite(pauseOpe_.get(), m);
        const bool hoverBack = HitTestSprite(pauseBackTitle_.get(), m);

        pauseOpe_->SetSize(hoverOpe ? hoverSize_ : opeBaseSize_);
        pauseBackTitle_->SetSize(hoverBack ? hoverSize_ : backBaseSize_);
        pauseOpe_->Update();
        pauseBackTitle_->Update();

        if (input->TriggerMouseButton(0)) {

            // backTitle：タイトルへ戻る
            if (hoverBack) {
                // ポーズ解除してから遷移（時間が0のままだと困るため）
                ExitPause();

                TransitionRequest req{};
                req.type = TransitionType::Fade;
                req.fadeOutSec = 0.3f;
                req.fadeInSec = 0.3f;

                SceneManager::GetInstance()->RequestChangeScene(titleSceneName_, req);
                return;
            }

            // ope：操作説明へ（menuは残す）
            if (hoverOpe) {
                pauseView_ = PauseView::Explain;
                return;
            }
        }
    }
}

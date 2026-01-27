#pragma once
#include "UIElement.h"
#include <functional>
#include <algorithm>

class TimeManager;

// シーン切替演出の共通ベース
// UIElementとしてUIManagerに登録して動かす前提
class SceneTransitionBase : public UIElement {
public:
    // 遷移進行フェーズ
    enum class Phase {
        None,     // 未開始
        FadeOut,  // 暗転中
        Hold,     // 暗転完了後の停止
        Switch,   // シーン切替の実行タイミング
        FadeIn,   // 明転中
        Done,     // 完了
    };

public:
    SceneTransitionBase();
    virtual ~SceneTransitionBase() = default;

    // 遷移開始
    // fadeOutSec : 暗転にかける秒数
    // fadeInSec  : 明転にかける秒数
    void Start(float fadeOutSec, float fadeInSec);

    // 暗転完了時に一度だけ呼ばれる
    void SetOnSwitch(std::function<void()> callback);

    // 明転完了時に呼ばれる
    void SetOnFinish(std::function<void()> callback);

    // 遷移中かどうか
    bool IsRunning() const;

    // 現在フェーズ取得（デバッグ用）
    Phase GetPhase() const { return phase_; }

    // UIElement
    void Update() override;

    // 暗転完了後に止める秒数（0なら停止なし）
    void SetHoldSeconds(float sec);

protected:
    // FadeOut: 0.0f -> 1.0f の進行度
    virtual void OnFadeOut(float progress01) = 0;

    // FadeIn : 0.0f -> 1.0f の進行度
    virtual void OnFadeIn(float progress01) = 0;

    // 経過時間取得（デフォルトはUnscaled）
    float GetDeltaTime() const;

    // 0..1 正規化
    static float Normalize(float t, float duration);

protected:
    Phase phase_ = Phase::None;

    float fadeOutSec_ = 0.0f;
    float fadeInSec_ = 0.0f;
    float timer_ = 0.0f;
    float holdSec_ = 0.0f;

    bool hasSwitched_ = false;

    std::function<void()> onSwitch_;
    std::function<void()> onFinish_;

    TimeManager* timeManager_ = nullptr;
};

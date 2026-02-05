#pragma once
#include <functional>
#include <memory>
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"

class UIManager;

//======================================================
// ISceneTransitionState
//  - 「遷移演出の作成＆UIManagerへAdd」を担当する State
//  - StartTransitionService が持つ "現在のState" を差し替えて使う
//======================================================
class ISceneTransitionState {
public:
    virtual ~ISceneTransitionState() = default;

    virtual void Enter(
        UIManager* ui,
        const TransitionRequest& req,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    ) = 0;
};


//------------------------------------------------------
// None（演出なし）State：即時切替・即終了
//------------------------------------------------------
class NoneTransitionState final : public ISceneTransitionState {
public:
    void Enter(
        UIManager* /*ui*/,
        const TransitionRequest& /*req*/,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    ) override
    {
        if (onSwitchOnce) onSwitchOnce();
        if (onFinishOnce) onFinishOnce();
    }
};

//------------------------------------------------------
// Fade State
//------------------------------------------------------
class FadeTransitionState final : public ISceneTransitionState {
public:
    void Enter(
        UIManager* ui,
        const TransitionRequest& req,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    ) override;
};

//------------------------------------------------------
// Shutter State
//------------------------------------------------------
class ShutterTransitionState final : public ISceneTransitionState {
public:
    void Enter(
        UIManager* ui,
        const TransitionRequest& req,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    ) override;
};

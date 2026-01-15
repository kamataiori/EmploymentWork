#pragma once
#include <functional>
#include <memory>
#include <string>

class TutorialController {
public:
    struct Actions {
        std::function<bool()> isMoving;
        std::function<bool()> jumpTriggered;
        std::function<bool()> attackTriggered;
        std::function<bool()> rollTriggered;
        std::function<bool()> dashTriggered;
    };

    struct Hooks {
        std::function<void(const std::string& spritePath)> onGuideChanged;
        std::function<void()> onFinished;
        std::function<void(float ratio)> onMoveGauge; // 0.0(空)〜1.0(満タン)

    };

    void Initialize(const Actions& actions, const Hooks& hooks);
    void Update(float dt);
    const char* GetStateName() const;

    void EmitMoveGauge(float ratio);


    struct IState {
        virtual ~IState() = default;
        virtual const char* Name() const = 0;
        virtual void Enter(TutorialController& ctx) {}
        virtual void Update(TutorialController& ctx, float dt) = 0;
        virtual void Exit(TutorialController& ctx) {}
    };

    template<class T, class... Args>
    void ChangeState(Args&&... args) {
        if (state_) { state_->Exit(*this); }
        state_ = std::make_unique<T>(std::forward<Args>(args)...);
        state_->Enter(*this);
    }

    void EmitGuide(const std::string& path) {
        if (hooks_.onGuideChanged) { hooks_.onGuideChanged(path); }
    }

    const Actions& Act() const { return actions_; }

    void Finish() {
        finished_ = true;
        if (hooks_.onFinished) { hooks_.onFinished(); }
    }

private:
    Actions actions_{};
    Hooks hooks_{};
    std::unique_ptr<IState> state_;
    bool finished_ = false;
};

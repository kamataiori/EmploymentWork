#include "TutorialController.h"
#include <utility>

namespace {

    //===============================
    // Safety Call Helper
    //===============================
    static bool Call(const std::function<bool()>& f) {
        return f ? f() : false;
    }

    //===============================
    // Forward Declarations
    //===============================
    struct MoveState;
    struct JumpCountState;
    struct AttackCountState;
    struct RollCountState;
    struct DashCountState;
    struct FinalMessageState;

    //===============================
    // 1) MoveState : WASD移動が連続で数秒
    //===============================
    struct MoveState final : TutorialController::IState {
        std::string guidePath_;
        float needSec_ = 2.0f;
        float acc_ = 0.0f;

        MoveState(std::string guidePath, float needSec)
            : guidePath_(std::move(guidePath)), needSec_(needSec) {
        }

        const char* Name() const override { return "MoveState"; }

        void Enter(TutorialController& ctx) override {
            acc_ = 0.0f;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float dt) override {
            // ★ ここが重要：isMoving() を呼ぶ（未設定なら false）
            if (Call(ctx.Act().isMoving)) {
                acc_ += dt;
            }
            else {
                // 連続判定にしたいので途切れたらリセット
                acc_ = 0.0f;
            }

            if (acc_ >= needSec_) {
                ctx.ChangeState<JumpCountState>("Resources/space_jump.png", 3);
            }
        }
    };

    //===============================
    // 2) JumpCountState : SPACEジャンプ3回
    //===============================
    struct JumpCountState final : TutorialController::IState {
        std::string guidePath_;
        int need_ = 3;
        int count_ = 0;

        JumpCountState(std::string guidePath, int need)
            : guidePath_(std::move(guidePath)), need_(need) {
        }

        const char* Name() const override { return "JumpCountState"; }

        void Enter(TutorialController& ctx) override {
            count_ = 0;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float) override {
            // ★ ここも重要：jumpTriggered() を呼ぶ
            if (Call(ctx.Act().jumpTriggered)) {
                ++count_;
            }
            if (count_ >= need_) {
                ctx.ChangeState<AttackCountState>("Resources/lmb_attack.png", 3);
            }
        }
    };

    //===============================
    // 3) AttackCountState : 左クリック攻撃3回
    //===============================
    struct AttackCountState final : TutorialController::IState {
        std::string guidePath_;
        int need_ = 3;
        int count_ = 0;

        AttackCountState(std::string guidePath, int need)
            : guidePath_(std::move(guidePath)), need_(need) {
        }

        const char* Name() const override { return "AttackCountState"; }

        void Enter(TutorialController& ctx) override {
            count_ = 0;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float) override {
            // ★ attackTriggered() を呼ぶ
            if (Call(ctx.Act().attackTriggered)) {
                ++count_;
            }
            if (count_ >= need_) {
                ctx.ChangeState<RollCountState>("Resources/e_roll.png", 3);
            }
        }
    };

    //===============================
    // 4) RollCountState : Eローリング3回
    //===============================
    struct RollCountState final : TutorialController::IState {
        std::string guidePath_;
        int need_ = 3;
        int count_ = 0;

        RollCountState(std::string guidePath, int need)
            : guidePath_(std::move(guidePath)), need_(need) {
        }

        const char* Name() const override { return "RollCountState"; }

        void Enter(TutorialController& ctx) override {
            count_ = 0;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float) override {
            // ★ rollTriggered() を呼ぶ
            if (Call(ctx.Act().rollTriggered)) {
                ++count_;
            }
            if (count_ >= need_) {
                ctx.ChangeState<DashCountState>("Resources/rmb_dash.png", 3);
            }
        }
    };

    //===============================
    // 5) DashCountState : 右クリックダッシュ3回
    //===============================
    struct DashCountState final : TutorialController::IState {
        std::string guidePath_;
        int need_ = 3;
        int count_ = 0;

        DashCountState(std::string guidePath, int need)
            : guidePath_(std::move(guidePath)), need_(need) {
        }

        const char* Name() const override { return "DashCountState"; }

        void Enter(TutorialController& ctx) override {
            count_ = 0;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float) override {
            // ★ dashTriggered() を呼ぶ
            if (Call(ctx.Act().dashTriggered)) {
                ++count_;
            }
            if (count_ >= need_) {
                ctx.ChangeState<FinalMessageState>("Resources/defeat_enemy.png", 1.5f);
            }
        }
    };

    //===============================
    // 6) FinalMessageState : 最後の文を一定秒表示→GameScene
    //===============================
    struct FinalMessageState final : TutorialController::IState {
        std::string guidePath_;
        float waitSec_ = 1.5f;
        float acc_ = 0.0f;

        FinalMessageState(std::string guidePath, float waitSec)
            : guidePath_(std::move(guidePath)), waitSec_(waitSec) {
        }

        const char* Name() const override { return "FinalMessageState"; }

        void Enter(TutorialController& ctx) override {
            acc_ = 0.0f;
            ctx.EmitGuide(guidePath_);
        }

        void Exit(TutorialController&) override {}

        void Update(TutorialController& ctx, float dt) override {
            acc_ += dt;
            if (acc_ >= waitSec_) {
                ctx.Finish();
            }
        }
    };

} // namespace

//==================================================
// TutorialController 本体
//==================================================
void TutorialController::Initialize(const Actions& actions, const Hooks& hooks)
{
    actions_ = actions;
    hooks_ = hooks;
    finished_ = false;

    // 最初：WASDで移動してみよう！＋マウス説明
    ChangeState<MoveState>("Resources/wasd_mouse.png", 2.0f);
}

void TutorialController::Update(float dt)
{
    if (finished_ || !state_) return;
    state_->Update(*this, dt);
}

const char* TutorialController::GetStateName() const
{
    return state_ ? state_->Name() : "None";
}

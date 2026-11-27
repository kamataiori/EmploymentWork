#pragma once

struct GameFlowContext;

//======================================================
// GameFlowState（基底クラス）
//------------------------------------------------------
// ゲームの「局面」1つ分。EnemyActionState と同じ Enter/Update/Exit の作りだが、
// あちらが「敵が今どの攻撃を実行中か」なのに対し、こちらは
// 「バトル全体が今どの段階か（登場演出／戦闘／決着）」を表す。
//
// 遷移は State 自身が要求する:
//   ctx.flow->ChangeState(std::make_unique<NextState>());
//
// 描画は局面ごとに出すものが違う（演出中はUIだけ、戦闘中はパーティクルも）ので、
// GamePlayScene の描画フェーズからそのまま委譲される。既定は「何も描かない」。
//======================================================
class GameFlowState
{
public:
    virtual ~GameFlowState() = default;

    // 局面に入った瞬間に1回
    virtual void Enter(GameFlowContext& ctx) { (void)ctx; }

    // 毎フレーム。dt は局面ごとに要否が違うので各Stateが TimeManager から取る
    virtual void Update(GameFlowContext& ctx) = 0;

    // 局面を抜ける瞬間に1回
    virtual void Exit(GameFlowContext& ctx) { (void)ctx; }

    // 前景スプライト（World層）
    virtual void ForeGroundDraw(GameFlowContext& ctx) { (void)ctx; }

    // パーティクル（Particle層）
    virtual void ParticleDraw(GameFlowContext& ctx) { (void)ctx; }

    // UI（合成後のバックバッファ＝ポストエフェクト対象外）
    virtual void UIDraw(GameFlowContext& ctx) { (void)ctx; }

    // デバッグ用：局面名
    virtual const char* GetName() const = 0;
};

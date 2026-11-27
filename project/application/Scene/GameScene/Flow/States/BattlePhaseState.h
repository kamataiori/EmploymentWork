#pragma once
#include "Flow/GameFlowState.h"

//======================================================
// BattlePhaseState（基底クラス）
//------------------------------------------------------
// 「世界が動いている」局面の共通部分。戦闘中も決着演出中も、
// ワールドの更新と当たり判定は動き続ける（敵の死亡アニメ、プレイヤーの
// 死亡モーション、その間のコライダー登録がそのまま必要なため）。
//
// 派生（BattleState / WinState / LoseState）は Update() の中で
// UpdateWorld() → RunCollisions() を呼んだ上に、自分の演出を足す。
//======================================================
class BattlePhaseState : public GameFlowState
{
public:
    void ForeGroundDraw(GameFlowContext& ctx) override;
    void ParticleDraw(GameFlowContext& ctx) override;
    void UIDraw(GameFlowContext& ctx) override;

protected:
    // ステージ・背景・キャラクターの更新
    void UpdateWorld(GameFlowContext& ctx);

    // その局面で有効なコライダーを登録して判定を回す。
    // 登録するものは局面によって変わり得るので、ここが一元の窓口になる
    // （例：コア戦を足したら、ボスは登録せずコアだけ登録する）
    void RunCollisions(GameFlowContext& ctx);
};

#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// AdvanceState（手前→奥へ移動）
//------------------------------------------------------
// 第1波（手前の円）を殲滅した後の局面。プレイヤーが細い道を通って
// 奥の四角エリアへ歩いて向かう間。奥エリア中心へ十分近づいたら
// 前哨戦（SentinelBattleState）を始める。
//
// この間も世界は動く（歩ける・ステージ押し戻しが効く）。敵は誰も動かない。
//======================================================
class AdvanceState : public BattlePhaseState
{
public:
    void Update(GameFlowContext& ctx) override;
    const char* GetName() const override { return "Advance"; }

private:
    // 奥エリア中心へのXZ距離がこれ以内になったら到達とみなす
    static constexpr float kArrivalRadius = 12.0f;
};

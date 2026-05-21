#pragma once

//======================================================
// FeintAttackParam
//------------------------------------------------------
// フェイント攻撃（読み合い技）のパラメータ
// WindUp(偽の溜め) → FeintHold(溜め持続＝誘い) → Lunge(本攻撃) → Recover(硬直)
//
// WindUp は SpinAttack と同じ構えに見せる。プレイヤーは
// 「回転が来る」と思い込むため、ここで早回避すると Lunge を食らう。
//======================================================
struct FeintAttackParam
{
    float windUpTime    = 0.40f;  // 偽の予備動作（本物の攻撃に見える溜め）
    float feintHoldTime = 0.40f;  // 構えたまま静止する時間。プレイヤーの早回避を誘う
    float lungeTime     = 0.22f;  // 本攻撃（突き込み）の時間。短い＝不意打ち
    float lungeDistance = 7.0f;   // 突き込みで前進する距離
    float hitRadius     = 5.0f;   // 突き込み中の攻撃判定の半径
    float recoverTime   = 0.95f;  // 硬直（プレイヤーの反撃チャンス）
    float turnLerp      = 0.30f;  // 溜め中にプレイヤーへ向き直る補間係数
};

#pragma once

//======================================================
// JumpSlamParam
//------------------------------------------------------
// ジャンプ急降下叩きつけ（間合いを詰める追撃技）のパラメータ
// WindUp(溜め) → Leap(放物線跳躍) → Impact(着地衝撃) → Recover(硬直)
//======================================================
struct JumpSlamParam
{
    float windUpTime   = 0.45f;  // 溜め（しゃがみ込み）。回避を判断する猶予
    float leapTime     = 0.70f;  // 跳躍開始〜着地までの時間
    float jumpHeight   = 12.0f;   // 放物線の頂点の高さ
    float maxLeapDist  = 22.0f;  // 最大跳躍距離（これ以上遠いと届かない＝逃げ切れる）
    float impactTime   = 0.22f;  // 着地後に攻撃判定を出している時間
    float impactRadius = 7.0f;   // 着地衝撃の範囲判定の半径
    float recoverTime  = 1.0f;   // 硬直（プレイヤーの反撃チャンス）

    //=== 着地時のカメラ振動 ===
    float shakeDuration  = 0.32f; // 振動の長さ（秒）
    float shakeAmplitude = 0.50f; // 振動の強さ（回転薙ぎ払いより強め＝重い着地）
    float shakeFrequency = 18.0f; // 振動の細かさ
};

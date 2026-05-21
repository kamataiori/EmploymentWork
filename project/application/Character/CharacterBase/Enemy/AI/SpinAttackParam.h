#pragma once

//======================================================
// SpinAttackParam
//------------------------------------------------------
// 回転薙ぎ払い（近接撃退技）のパラメータ
// WindUp(予備動作) → Spin(回転攻撃) → Recover(硬直) で構成
//======================================================
struct SpinAttackParam
{
    float windUpTime  = 0.5f;   // 溜め（予備動作）。プレイヤーが離脱を判断する猶予
    float spinTime    = 0.7f;   // 回転攻撃の持続時間（攻撃判定が出ている時間）
    float recoverTime = 0.9f;   // 硬直（プレイヤーの反撃チャンス）
    float spinSpeed   = 18.0f;  // 回転速度 [rad/s]
    float hitRadius   = 6.0f;   // 攻撃判定（球）の半径。近距離を危険地帯にする
    float turnLerp    = 0.3f;   // 溜め中にプレイヤーへ向き直る補間係数

    //=== 回転中のホップ（浮き上がり→着地）===
    float liftHeight   = 1.6f;  // 回転中に浮く高さ。控えめにして重量感を保つ
    float liftRiseTime = 0.18f; // 浮き上がりにかける時間（イーズアウト＝フワッと）
    float liftFallTime = 0.12f; // 着地にかける時間（イーズイン＝ドスンと重く落とす）

    //=== 着地時のカメラ振動 ===
    float shakeDuration  = 0.28f; // 振動の長さ（秒）
    float shakeAmplitude = 0.35f; // 振動の強さ（カメラの揺れ幅）
    float shakeFrequency = 16.0f; // 振動の細かさ（大きいほどブルブル）
};

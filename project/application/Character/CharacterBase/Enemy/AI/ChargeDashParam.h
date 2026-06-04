#pragma once

struct ChargeDashParam
{
    float chargeTime = 0.8f;   // 溜め
    float dashDistance = 20.0f;  // 距離
    float dashSpeed = 25.0f;  // 速度
    float recoverTime = 0.4f;   // 硬直
    float cooldownTime = 1.2f;   // クールダウン
    float turnLerp = 0.25f;  // 溜め中の旋回補間
};
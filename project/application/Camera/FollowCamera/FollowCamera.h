#pragma once
#include "Camera.h"
#include "CharacterBase.h"

class FollowCamera : public Camera
{
public:
    FollowCamera(CharacterBase* target, float distance = 10.0f, float height = 2.0f, float horizontalOffset = -5.0f);

    void Update() override;

    void SetDistance(float d) { distance_ = d; }
    void SetHeight(float h) { height_ = h; }
    void SetHorizontalOffset(float offset) { horizontalOffset_ = offset; }

    void Debug();

private:
    CharacterBase* target_;
    float distance_;          // プレイヤーとの距離（Z方向）
    float height_;            // 高さオフセット（Y方向）
    float horizontalOffset_;  // 横ずらし（X方向）
};

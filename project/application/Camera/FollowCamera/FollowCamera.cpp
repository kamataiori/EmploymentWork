#include "FollowCamera.h"
#include <Input.h>

FollowCamera::FollowCamera(CharacterBase* target, float followDistance, float heightOffset)
    : target(target), followDistance(followDistance), heightOffset(heightOffset)
{
}

void FollowCamera::Update()
{
    if (!target) return;

    // ← → キーでカメラ回転
    if (Input::GetInstance()->PushKey(DIK_LEFT)) {
        angle -= 0.03f;
    }
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
        angle += 0.03f;
    }

    const Vector3& targetPos = target->GetTransform().translate;

    // プレイヤーの向きを基準に後ろ側を計算（Y=3.14 で後ろにいるように）
    Vector3 offset = {
        std::sin(angle) * followDistance,
        heightOffset,
        std::cos(angle) * followDistance
    };

    Vector3 desiredPos = targetPos + offset;

    // カメラを滑らかに補間移動
    float smoothSpeed = 0.03f;
    transform.translate = Lerp(transform.translate, desiredPos, smoothSpeed);

    // カメラが常にプレイヤーの方向を向くように回転
    Vector3 direction = Normalize(targetPos - transform.translate);
    transform.rotate.y = std::atan2(direction.x, direction.z);

    Camera::Update();
}


#include "FollowCamera.h"

FollowCamera::FollowCamera(CharacterBase* target, float followDistance, float heightOffset)
    : target(target), followDistance(followDistance), heightOffset(heightOffset)
{
}

void FollowCamera::Update()
{
    if (target) // ターゲットが存在する場合のみ追従
    {
        const Vector3& targetPosition = target->GetTransform().translate; // ターゲットの位置を取得

        // 対象との方向を計算
        Vector3 direction = Normalize(transform.translate - targetPosition);

        // 距離と高さを考慮してカメラ位置を設定
        transform.translate = targetPosition + direction * followDistance;
        transform.translate.y += heightOffset; // 高さを反映
    }

    // 基底クラスの Update を呼び出す
    Camera::Update();
}

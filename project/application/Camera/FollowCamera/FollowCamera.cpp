#include "FollowCamera.h"
#include <Input.h>

FollowCamera::FollowCamera(ObjectBase* target, float followDistance, float heightOffset)
    : target(target), followDistance(followDistance), heightOffset(heightOffset)
{
}

void FollowCamera::Update()
{
    if (!target) return;

    float dt = TimeManager::GetInstance()->GetDeltaTime();

    if (Input::GetInstance()->PushKey(DIK_LEFT))  angle -= 0.03f;
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) angle += 0.03f;

    angle += Input::GetInstance()->GetMouseDelta().x * sensitivity_;

    const Vector3& targetPos = target->GetTransform().translate;

    Vector3 offset = {
        std::sin(angle) * followDistance,
        heightOffset,
        std::cos(angle) * followDistance
    };
    Vector3 desiredPos = targetPos + offset;

    // カメラ位置は一旦そのまま（後でY追従弱めも可能）
    // transform.translate = desiredPos;
    // XZは即追従、Yだけ遅らせる
    transform.translate.x = desiredPos.x;
    transform.translate.z = desiredPos.z;

    // 遅らせて追従
    if (dampPosY_) {
        const float t = 1.0f - std::exp(-posYSmooth_ * dt);
        transform.translate.y = transform.translate.y + (desiredPos.y - transform.translate.y) * t;
    }
    else {
        transform.translate.y = desiredPos.y;
    }


    // -----------------------------
    // 注視点Yを固定 or 緩める
    // -----------------------------
    Vector3 lookAt = targetPos;

    if (lockLookY_) {
        // 初回だけ現在の高さを基準に固定（地面基準にしたいなら 0.0f + 任意オフセットでもOK）
        static bool initialized = false;
        if (!initialized) {
            lookY_ = targetPos.y;
            initialized = true;
        }
        lookAt.y = lookY_;
    }
    else {
        // ゆっくり追従（指数補間：dt対応）
        const float t = 1.0f - std::exp(-lookYSmooth_ * dt);
        lookY_ = lookY_ + (targetPos.y - lookY_) * t;
        lookAt.y = lookY_;
    }

    // 向き：lookAt（Y固定）へ向ける
    Vector3 dir = Normalize(lookAt - transform.translate);
    transform.rotate.y = std::atan2(dir.x, dir.z);

    // Pitchも lookAt に合わせたいならこれ（今は固定下向きでもOK）
    // transform.rotate.x = std::atan2(-dir.y, std::sqrt(dir.x*dir.x + dir.z*dir.z));

    // 固定で下向きにしたいならこれでもOK
    transform.rotate.x = 0.25f;

    Camera::Update();
}


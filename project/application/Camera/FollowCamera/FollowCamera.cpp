#include "FollowCamera.h"
#include <Input.h>

FollowCamera::FollowCamera(ObjectBase* target, float followDistance, float heightOffset)
	: target(target),
	followDistance(followDistance),
	heightOffset(heightOffset),
	shoulderOffset(3.5f),   // 2.8f → 3.5f：右肩越しを強調
	sensitivity_(0.005f),
	dampPosY_(true),
	posYSmooth_(12.0f),     // 6.0f → 12.0f：Y追従を速くしてカメラが落ち着く
	lockLookY_(false),
	lookYSmooth_(10.0f),
	lookY_(0.0f),
	angle(0.0f)
{
}

void FollowCamera::Update()
{
    if (!target) return;

    float dt = TimeManager::GetInstance()->GetDeltaTime();

    // 初回だけ：カメラの周回角を player の向きに合わせる
    static bool sInitializedAngle = false;
	if (!initializedAngle_) {
		angle = target->GetTransform().rotate.y;
		initializedAngle_ = true;
	}

    // =============================
    // 入力：周回角（カメラオービット）
    // =============================
    if (Input::GetInstance()->PushKey(DIK_LEFT))  angle -= 0.03f;
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) angle += 0.03f;

    angle += Input::GetInstance()->GetMouseDelta().x * sensitivity_;

    const auto& t = target->GetTransform();
    const Vector3& targetPos = t.translate;

    // playerの向き（Yaw）を基準に「右方向」を作る
    // ※もし player の yaw が別管理ならここを差し替え
    const float targetYaw = t.rotate.y;

    // =============================
    // カメラ位置
    //   - 後ろ方向：カメラ周回 angle 基準
    //   - 肩オフセット：playerYaw 基準（回り込み防止）
    // =============================
    const float bs = std::sin(angle);
    const float bc = std::cos(angle);

    // player基準の右方向
    const float rs = std::sin(targetYaw);
    const float rc = std::cos(targetYaw);

    Vector3 offset = {
        bs * followDistance + rc * shoulderOffset,
        heightOffset,
        bc * followDistance - rs * shoulderOffset
    };

	Vector3 desiredPos = targetPos + offset;

	// XZは即追従
	transform.translate.x = desiredPos.x;
	transform.translate.z = desiredPos.z;

	if (dampPosY_) {
		// 初回だけ補間せずに直接セット
		if (!initializedPosY_) {
			transform.translate.y = desiredPos.y;
			lookY_ = targetPos.y;
			initializedPosY_ = true;
		}
		else {
			const float tt = 1.0f - std::exp(-posYSmooth_ * dt);
			transform.translate.y += (desiredPos.y - transform.translate.y) * tt;
		}
	}
	else {
		transform.translate.y = desiredPos.y;
	}

    // =============================
    // 注視点Y：固定 or 緩め追従
    // =============================
    Vector3 lookAt = targetPos;

    if (lockLookY_) {
        static bool initialized = false;
        if (!initialized) {
            lookY_ = targetPos.y;
            initialized = true;
        }
        lookAt.y = lookY_;
    }
    else {
        const float tt = 1.0f - std::exp(-lookYSmooth_ * dt);
        lookY_ = lookY_ + (targetPos.y - lookY_) * tt;
        lookAt.y = lookY_;
    }

    // =============================
    // 見る位置：胸 + 少し左（playerYaw基準に統一）
    // =============================
    lookAt.y += 1.2f; // 胸あたり

    // 「少し左を見る」＝ player基準で左にずらす
    // ※右肩カメラでキャラを画面左寄せにしたいので "左" を見る
    float sideOffset = 0.6f;

    // left = -right
    lookAt.x -= rc * sideOffset;
    lookAt.z += rs * sideOffset;

    // =============================
    // 向き：Yaw + Pitch を lookAt に合わせる
    // =============================
    Vector3 dir = Normalize(lookAt - transform.translate);

    // Yaw
    transform.rotate.y = std::atan2(dir.x, dir.z);

    // Pitch（反転しない範囲に制限）
    float horizontalLength = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    float pitch = std::atan2(-dir.y, horizontalLength);

    const float minPitch = 0.10f; // 少し下向き（約6度）
    const float maxPitch = 0.60f; // 下向き最大（約34度）

    if (pitch < minPitch) pitch = minPitch;
    if (pitch > maxPitch) pitch = maxPitch;

    transform.rotate.x = pitch;

    Camera::Update();
}
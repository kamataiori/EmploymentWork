#include "FollowCamera.h"
#include <Input.h>

FollowCamera::FollowCamera(ObjectBase* target, float followDistance, float heightOffset)
	: target(target),
	followDistance(followDistance),
	heightOffset(heightOffset),
	shoulderOffset(0.0f),
	sensitivity_(0.005f),
	dampPosY_(true),
	posYSmooth_(12.0f),
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

	// 初回だけ：プレイヤーの後ろ（+π）にカメラを配置
	if (!initializedAngle_) {
		angle = target->GetTransform().rotate.y + std::numbers::pi_v<float>;
		initializedAngle_ = true;
	}

	// =============================
	// 入力：周回角（カメラオービット）
	// =============================
	if (Input::GetInstance()->PushKey(DIK_LEFT))  angle -= 0.03f;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) angle += 0.03f;

	angle += Input::GetInstance()->GetMouseDelta().x * sensitivity_;

	const Vector3& targetPos = target->GetTransform().translate;

	// カメラ位置計算の部分を変更
	// カメラを左にずらす（マイナス方向）
	const float camShiftX = -std::cos(angle) * 2.2f;
	const float camShiftZ = std::sin(angle) * 2.2f;

	Vector3 desiredPos = {
		targetPos.x + std::sin(angle) * followDistance + camShiftX,
		targetPos.y + heightOffset,
		targetPos.z + std::cos(angle) * followDistance + camShiftZ
	};

	// XZ は即追従
	transform.translate.x = desiredPos.x;
	transform.translate.z = desiredPos.z;

	// Y だけ遅らせる（初回は即セット）
	if (dampPosY_) {
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
	// 注視点：プレイヤーの胸あたりを見る
	// =============================
	Vector3 lookAt = targetPos;

	// 注視点 Y を滑らかに追従
	{
		const float tt = 1.0f - std::exp(-lookYSmooth_ * dt);
		lookY_ = lookY_ + (targetPos.y - lookY_) * tt;
		lookAt.y = lookY_ + 0.5f; // 胸の高さ
	}

	// =============================
	// 肩越し（右肩カメラ）：
	// 注視点を「カメラの右方向」にずらすことでキャラを画面左寄りに見せる
	// カメラ位置は円のまま変えないので距離は絶対変わらない
	// =============================
	// angle に対する右方向（forward を -90° 回転）
	const float rightX = std::cos(angle);
	const float rightZ = -std::sin(angle);

	// 注視点も同じ方向（左）にずらす
	lookAt.x += camShiftX;
	lookAt.z += camShiftZ;

	// =============================
	// 向き：lookAt に向ける（Yaw + Pitch）
	// =============================
	Vector3 dir = Normalize(lookAt - transform.translate);

	transform.rotate.y = std::atan2(dir.x, dir.z);

	const float horizontalLength = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	float pitch = std::atan2(-dir.y, horizontalLength);

	// Pitch クランプ（下向きのみ）
	pitch = std::clamp(pitch, 0.60f, 0.65f);
	transform.rotate.x = pitch;

	Camera::Update();
}
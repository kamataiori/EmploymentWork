#include "FollowCamera.h"
#include <Input.h>

FollowCamera::FollowCamera(ObjectBase* target, float followDistance, float heightOffset)
	: target(target),
	followDistance(followDistance),
	heightOffset(heightOffset),
	sensitivity_(0.0025f),
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

	// 初回だけ：初期の高さ(heightOffset)から見下ろし角を逆算して合わせる
	if (!initializedPitch_) {
		cameraPitch_ = std::atan2(heightOffset - lookHeight_, followDistance);
		cameraPitch_ = std::clamp(cameraPitch_, pitchMin_, pitchMax_);
		initializedPitch_ = true;
	}

	// =============================
	// 入力：周回角（カメラオービット）
	// =============================
	if (Input::GetInstance()->PushKey(DIK_LEFT))  angle -= keyOrbitSpeed_;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) angle += keyOrbitSpeed_;

	// マウス操作が許可されている間だけカメラを動かす。
	// （スキル発動中など mouseControlEnabled_=false でマウスのカメラ操作を止める）
	// パッドの右スティックもマウスと同じ扱いにする（同じロック・同じ感度スケールに従う）。
	if (mouseControlEnabled_) {
		// 左右：水平の周回角（スロー演出中は感度スケールでゆっくりにする）
		angle += Input::GetInstance()->GetMouseDelta().x * sensitivity_ * mouseSensitivityScale_;
		angle += Input::GetInstance()->GetRightStickX() * padOrbitSpeed_ * mouseSensitivityScale_;

		// 上下：見下ろし角（マウスを下げると見下ろし、上げると見上げ）
		cameraPitch_ += Input::GetInstance()->GetMouseDelta().y * pitchSensitivity_ * mouseSensitivityScale_;
		// スティックは上が正。マウスと向きを揃えるため符号を反転する（倒し上げ＝見上げ）
		cameraPitch_ -= Input::GetInstance()->GetRightStickY() * padPitchSpeed_ * mouseSensitivityScale_;
		cameraPitch_ = std::clamp(cameraPitch_, pitchMin_, pitchMax_);
	}

	const Vector3& targetPos = target->GetTransform().translate;

	// カメラ位置計算の部分を変更
	// カメラを左にずらす（マイナス方向）
	const float camShiftX = -std::cos(angle) * cameraSideShift_;
	const float camShiftZ = std::sin(angle) * cameraSideShift_;

	// 縦方向：注視点の高さを基準に、見下ろし角ぶんだけカメラを持ち上げる。
	// これでマウス上下に応じてカメラが縦に周回する。
	const float camHeight = lookHeight_ + followDistance * std::tan(cameraPitch_);

	Vector3 desiredPos = {
		targetPos.x + std::sin(angle) * followDistance + camShiftX,
		targetPos.y + camHeight,
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
	// 注視点：プレイヤーの頭あたりを見る
	// =============================
	Vector3 lookAt = targetPos;

	// 注視点 Y を滑らかに追従
	{
		const float tt = 1.0f - std::exp(-lookYSmooth_ * dt);
		lookY_ = lookY_ + (targetPos.y - lookY_) * tt;
		lookAt.y = lookY_ + lookHeight_; // 画面中央に映る高さ（頭の高さ）
	}

	// =============================
	// 肩越し（右肩カメラ）：
	// 注視点を横方向にずらすことでキャラを画面端寄りに見せる
	// カメラ位置は円のまま変えないので距離は絶対変わらない
	// =============================
	lookAt.x += camShiftX;
	lookAt.z += camShiftZ;

	// =============================
	// シネマティック上書き：通常追従ポーズと演出ポーズを weight で補間する
	// （アルティメットの見下ろし演出などで使用。weight=0 なら完全に通常追従）
	// =============================
	const bool cinematic = (cineWeight_ > 0.0001f);
	if (cinematic) {
		const float w = std::clamp(cineWeight_, 0.0f, 1.0f);
		transform.translate.x += (cinePos_.x - transform.translate.x) * w;
		transform.translate.y += (cinePos_.y - transform.translate.y) * w;
		transform.translate.z += (cinePos_.z - transform.translate.z) * w;
		lookAt.x += (cineLook_.x - lookAt.x) * w;
		lookAt.y += (cineLook_.y - lookAt.y) * w;
		lookAt.z += (cineLook_.z - lookAt.z) * w;
	}

	// =============================
	// 向き：lookAt に向ける（Yaw + Pitch）
	// =============================
	Vector3 dir = Normalize(lookAt - transform.translate);

	transform.rotate.y = std::atan2(dir.x, dir.z);

	const float horizontalLength = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	float pitch = std::atan2(-dir.y, horizontalLength);

	// Pitch クランプ（下向きのみ）。ただし見下ろし演出中は真下も向けるようクランプしない。
	if (!cinematic) {
		pitch = std::clamp(pitch, pitchMin_, pitchMax_);
	}
	transform.rotate.x = pitch;

	Camera::Update();
}
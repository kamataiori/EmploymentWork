#include "FollowCamera.h"
#include "ImGuiManager.h"

FollowCamera::FollowCamera(ObjectBase* target, float followDistance, float heightOffset)
	: target(target),
	followDistance(followDistance),
	heightOffset(heightOffset),
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
	// 入力：周回角（カメラオービット）／見下ろし角
	//   どのデバイスをどの感度で使うかは入力側（ICameraInput の実装）が持つ。
	//   ここで決めるのは「カメラ側の方針」だけ:
	//     ・キーボードの旋回は一定速度で常に効く
	//     ・マウス/スティックはロックで止められ、感度スケールで鈍らせられる
	//     ・見下ろし角は可動域でクランプする
	// =============================
	if (cameraInput_) {
		const LookAxisValue yawLook = cameraInput_->GetYawLook();
		const LookAxisValue pitchLook = cameraInput_->GetPitchLook();

		angle += yawLook.keyDelta;

		// （スキル発動中など mouseControlEnabled_=false で視点操作を止める）
		if (mouseControlEnabled_) {
			// 左右：水平の周回角（スロー演出中は感度スケールでゆっくりにする）
			angle += yawLook.pointerDelta * mouseSensitivityScale_;

			// 上下：見下ろし角（マウスを下げると見下ろし、上げると見上げ）
			cameraPitch_ += pitchLook.pointerDelta * mouseSensitivityScale_;
			cameraPitch_ = std::clamp(cameraPitch_, pitchMin_, pitchMax_);
		}
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

	// =============================
	// めり込み防止（スプリングアーム）
	//   注視点(lookAt)からカメラへ伸ばす「腕」を、壁に当たる手前まで縮める。
	//   ・線ではなく球で掃くので、ニアクリップ面が壁へ入る前に止まる
	//   ・縮めるのは即時／戻すのはゆっくり（逆にすると壁際でカメラが震える）
	//   ・腕の方向は変えないので、映っている向きは変わらない
	//   ・見下ろし演出中は壁を抜けて移動するので判定しない
	// =============================
	UpdateCollisionPullIn(lookAt, cinematic, dt);
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

void FollowCamera::UpdateCollisionPullIn(const Vector3& pivot, bool cinematic, float dt)
{
	// 腕の長さが取れないほど近いときは何もしない
	constexpr float kArmEpsilon = 0.0001f;

	const Vector3 arm = transform.translate - pivot;
	const float fullDistance = Length(arm);
	if (fullDistance <= kArmEpsilon) return;

	// 演出中・無効時は腕を伸ばし切った状態にしておく。
	// （縮めた長さを覚えたままにすると、演出明けにカメラが飛ぶ）
	if (cinematic || !collisionEnabled_ || !cameraBlocker_) {
		collisionDistance_ = fullDistance;
		initializedCollisionDistance_ = true;
		return;
	}

	const Vector3 armDir = arm / fullDistance;

	// このフレームに許される腕の長さ
	float allowedDistance = fullDistance;
	Vector3 hitPos{};
	if (cameraBlocker_->SweepCamera(pivot, transform.translate, collisionRadius_, hitPos)) {
		allowedDistance = Length(hitPos - pivot) - collisionEscape_;
	}
	// 詰めの下限。腕が下限より短い状況（極端な寄り演出など）でも
	// 下限＞上限にならないよう、腕の長さ側に合わせておく（clamp は lo>hi で未定義動作）
	const float minDistance = (std::min)(collisionMinDistance_, fullDistance);
	allowedDistance = std::clamp(allowedDistance, minDistance, fullDistance);

	if (!initializedCollisionDistance_) {
		collisionDistance_ = allowedDistance;
		initializedCollisionDistance_ = true;
	}
	else if (allowedDistance < collisionDistance_) {
		// 壁に入りそうなときは即座に縮める（1フレームでもめり込ませない）
		collisionDistance_ = allowedDistance;
	}
	else {
		// 壁から離れたらゆっくり戻す（即時に戻すとカメラが飛ぶ・壁際で震える）
		const float tt = 1.0f - std::exp(-collisionReturnSharp_ * dt);
		collisionDistance_ += (allowedDistance - collisionDistance_) * tt;
	}
	collisionDistance_ = std::clamp(collisionDistance_, minDistance, fullDistance);

	// 腕の方向は変えずに長さだけ差し替える
	transform.translate = pivot + armDir * collisionDistance_;
}

void FollowCamera::DebugImGui()
{
#ifdef USE_IMGUI
	// スライダーの範囲
	constexpr float kRadiusMin = 0.05f;   // 掃く球の半径[m]
	constexpr float kRadiusMax = 1.50f;
	constexpr float kEscapeMax = 0.50f;   // 壁側に残す余白[m]
	constexpr float kMinDistMin = 0.10f;  // 詰めの下限[m]
	constexpr float kMinDistMax = 5.00f;
	constexpr float kReturnMin = 1.00f;   // 戻す速さ[1/秒]
	constexpr float kReturnMax = 30.0f;

	ImGui::Begin("FollowCamera Collision");

	ImGui::Text("blocker : %s", cameraBlocker_ ? "injected" : "none");
	ImGui::Text("arm now : %.2f m", collisionDistance_);
	ImGui::Separator();

	ImGui::Checkbox("enabled", &collisionEnabled_);
	ImGui::SliderFloat("radius [m]", &collisionRadius_, kRadiusMin, kRadiusMax);
	ImGui::SliderFloat("escape [m]", &collisionEscape_, 0.0f, kEscapeMax);
	ImGui::SliderFloat("min dist [m]", &collisionMinDistance_, kMinDistMin, kMinDistMax);
	ImGui::SliderFloat("return [1/s]", &collisionReturnSharp_, kReturnMin, kReturnMax);

	ImGui::End();
#endif
}
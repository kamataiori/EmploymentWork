#include "FPSCamera.h"

FPSCamera::FPSCamera(CharacterBase* target, float eyeHeight)
	: target(target), eyeHeight(eyeHeight), updateMode(UpdateMode::Normal)
{
}

void FPSCamera::Update()
{
	if (!target)
	{
		Camera::Update();
		return;
	}

	switch (updateMode)
	{
	case UpdateMode::Normal:
		UpdateNormal();
		break;
	case UpdateMode::ClampedRotation:
		UpdateClampedRotation();
		break;
	case UpdateMode::Easing:
		UpdateEasing();
		break;
	case UpdateMode::Shaky:
		UpdateShaky();
		break;
	}

	Camera::Update(); // 基底クラスの Update を呼び出す
}

void FPSCamera::UpdateNormal()
{
	//-----ノーマルなカメラUpdate-----//

	const Vector3& targetPosition = target->GetTransform().translate; // ターゲットの位置を取得
	const Vector3& targetRotation = target->GetTransform().rotate;   // ターゲットの回転を取得

	// プレイヤーの目線の高さにカメラを配置
	transform.translate = targetPosition;
	transform.translate.y += eyeHeight;

	// ターゲットの回転をカメラに反映
	transform.rotate = targetRotation;
}

void FPSCamera::UpdateClampedRotation()
{
	//-----視点回転の制限付きカメラUpdate-----//

	const Vector3& targetPosition = target->GetTransform().translate;
	const Vector3& targetRotation = target->GetTransform().rotate;

	// カメラ位置を更新
	transform.translate = targetPosition;
	transform.translate.y += eyeHeight;

	// 視点の上下回転を制限（-89°～89°）
	transform.rotate.x = std::clamp(targetRotation.x, -89.0f, 89.0f);
	transform.rotate.y = targetRotation.y;

	// カメラのロール回転を固定（必要なら）
	transform.rotate.z = 0.0f;
}

void FPSCamera::UpdateEasing()
{
	//-----イージングによるカメラUpdate-----//

	const Vector3& targetPosition = target->GetTransform().translate;
	const Vector3& targetRotation = target->GetTransform().rotate;

	// 緩やかに位置を追従（イージング）
	float followSpeed = 0.1f;
	transform.translate.x += (targetPosition.x - transform.translate.x) * followSpeed;
	transform.translate.y += (targetPosition.y + eyeHeight - transform.translate.y) * followSpeed;
	transform.translate.z += (targetPosition.z - transform.translate.z) * followSpeed;

	// 回転を緩やかに追従
	transform.rotate.y += (targetRotation.y - transform.rotate.y) * followSpeed;
	transform.rotate.x += (targetRotation.x - transform.rotate.x) * followSpeed;
}

void FPSCamera::UpdateShaky()
{
	//-----リアルなカメラの手振れ効果付きカメラUpdate-----//

		//const Vector3& targetPosition = target->GetTransform().translate;

		//// 基本位置
		//transform.translate = targetPosition;
		//transform.translate.y += eyeHeight;

		//// 手ぶれ効果（例: サイン波で揺らす）
		//float shakeStrength = 0.1f;
		//transform.translate.x += std::sin(Time::GetElapsedTime() * 10.0f) * shakeStrength;
		//transform.translate.y += std::cos(Time::GetElapsedTime() * 10.0f) * shakeStrength;
}

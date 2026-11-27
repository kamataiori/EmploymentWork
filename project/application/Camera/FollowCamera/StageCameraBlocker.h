#pragma once
#include "ICameraBlocker.h"

class MultiCollider;

//======================================================
// StageCameraBlocker
//------------------------------------------------------
// ステージのメッシュ当たり判定（SceneController::BuildStageCollider が
// ワールド空間へ焼いた MeshBVH）を、カメラの遮蔽判定として貸し出すアダプタ。
//
// 判定対象をステージだけに限るための層でもある:
//   敵やプレイヤーにも反応させると、敵が横切るたびにカメラが寄って酔うため、
//   ここでステージのコライダーだけを見るようにしている。
//
// 所有はしない（ステージのコライダーは Scene が持つ）。
//======================================================
class StageCameraBlocker : public ICameraBlocker
{
public:
	/// <summary>
	/// 遮蔽に使うステージのコライダーを渡す（所有しない）。
	/// SceneController::BuildStageCollider() の後に呼ぶこと。
	/// </summary>
	void SetStageCollider(const MultiCollider* collider) { stageCollider_ = collider; }

	bool SweepCamera(const Vector3& pivot, const Vector3& desired,
		float radius, Vector3& outHitPos) const override;

private:
	// 所有はしない
	const MultiCollider* stageCollider_ = nullptr;
};

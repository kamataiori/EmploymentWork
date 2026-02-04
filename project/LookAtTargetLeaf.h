#pragma once

#include "application/AI/BehaviorTree/Core/NodeBase.h"
#include "application/AI/BehaviorTree/Core/BlackBoard.h"

#include "Enemy/Enemy.h"

#include <cmath>

//======================================================
// LookAtTargetLeaf
// ・毎フレーム target の方向へ向くだけ（移動しない）
// ・成功：target が有効で回転更新できた
// ・失敗：target が取れない
//======================================================
class LookAtTargetLeaf : public NodeBase
{
public:
	explicit LookAtTargetLeaf(BlackBoard* bb, float turnLerp = 0.2f)
		: NodeBase(bb), turnLerp_(turnLerp) {
	}

protected:
	void tick() override
	{
		if (!mpBlackBoard || !mpBlackBoard->has_key("enemy") || !mpBlackBoard->has_key("target")) {
			mNodeResult = NodeResult::Fail;
			return;
		}

		Enemy* enemy = nullptr;
		const Transform* target = nullptr;

		try {
			enemy = mpBlackBoard->get_value<Enemy*>("enemy");
			target = mpBlackBoard->get_value<const Transform*>("target");
		}
		catch (...) {
			mNodeResult = NodeResult::Fail;
			return;
		}

		if (!enemy || !target) {
			mNodeResult = NodeResult::Fail;
			return;
		}

		Transform t = enemy->GetTransform();

		// 方向（XZ）
		Vector3 to = target->translate - t.translate;
		to.y = 0.0f;

		const float len = std::sqrt(to.x * to.x + to.z * to.z);
		if (len <= 1e-6f) {
			mNodeResult = NodeResult::Success;
			return;
		}

		const float inv = 1.0f / len;
		Vector3 dir = { to.x * inv, 0.0f, to.z * inv };

		const float desiredYaw = std::atan2(dir.x, dir.z);

		// 角度差を [-pi, pi] に
		auto Wrap = [](float a) {
			while (a > 3.14159265f)  a -= 6.2831853f;
			while (a < -3.14159265f) a += 6.2831853f;
			return a;
			};

		const float delta = Wrap(desiredYaw - t.rotate.y);
		t.rotate.y = t.rotate.y + delta * turnLerp_;

		enemy->SetTransform(t);

		mNodeResult = NodeResult::Success; // “向く” は完了扱いでOK（毎フレーム繰り返される）
	}

private:
	float turnLerp_ = 0.2f;
};

#include "TargetPicker.h"

#include <algorithm>
#include <cmath>

ITarget* TargetPicker::Pick(IEnemyTargetProvider* provider,
	const Vector3& fromPos, const Vector3& basisDirXZ)
{
	found_ = false;
	lastAngleDeg_ = 0.0f;
	lastDistance_ = 0.0f;

	if (!provider) return nullptr;

	// 基準方向を水平面で正規化（長さが無ければ向きが決まらない）
	const float basisLength = std::sqrt(basisDirXZ.x * basisDirXZ.x + basisDirXZ.z * basisDirXZ.z);
	if (basisLength <= kDirectionEpsilon_) return nullptr;
	const float basisX = basisDirXZ.x / basisLength;
	const float basisZ = basisDirXZ.z / basisLength;

	scratch_.clear();
	provider->CollectAliveTargets(scratch_);

	ITarget* best = nullptr;
	float bestScore = 0.0f;
	float bestAngleDeg = 0.0f;
	float bestDistance = 0.0f;

	for (ITarget* target : scratch_) {
		if (!target || !target->IsAlive()) continue;

		// 高さは無視して水平面で測る（向きはヨー角しか変えないため）
		const Vector3 to = target->GetTargetCenter() - fromPos;
		const float distance = std::sqrt(to.x * to.x + to.z * to.z);

		if (distance <= kDirectionEpsilon_) continue;    // 真上・真下は方向が決まらない
		if (distance > params_.maxDistance) continue;    // 射程外

		// 基準方向とのなす角
		const float cosAngle = (to.x * basisX + to.z * basisZ) / distance;
		const float angleDeg = std::acos(std::clamp(cosAngle, -1.0f, 1.0f)) * kRadToDeg_;
		if (angleDeg > params_.maxAngleDeg) continue;    // 向けている方向から外れすぎ

		// 角度を距離より優先したスコア（小さいほど良い）
		const float score = angleDeg * params_.angleWeight + distance * params_.distanceWeight;
		if (!best || score < bestScore) {
			best = target;
			bestScore = score;
			bestAngleDeg = angleDeg;
			bestDistance = distance;
		}
	}

	if (best) {
		found_ = true;
		lastAngleDeg_ = bestAngleDeg;
		lastDistance_ = bestDistance;
	}
	return best;
}

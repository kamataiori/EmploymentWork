#include "StageCameraBlocker.h"

#include "MultiCollider.h"
#include "ShapeIntersect.h"

bool StageCameraBlocker::SweepCamera(const Vector3& pivot, const Vector3& desired,
	float radius, Vector3& outHitPos) const
{
	if (!stageCollider_) return false;

	// ステージのメッシュ形状だけを見る（球やカプセルの敵コライダーは無視）
	for (const Shape& shape : stageCollider_->GetShapes()) {
		if (shape.kind != ShapeKind::Mesh) continue;

		// 1) 腕の根元から本来置きたい位置へ球を掃く（最初に触れた位置が返る）
		if (SweepSphereVsMesh(shape, pivot, desired, radius, outHitPos)) {
			return true;
		}

		// 2) 保険：SweepSphereVsMesh は移動量が刻み幅以下だと判定しない仕様なので、
		//    腕が短いときのために「置きたい位置がすでに埋まっていないか」も見る。
		Shape probe{ ShapeKind::Sphere };
		probe.sphere.center = desired;
		probe.sphere.radius = radius;
		if (Intersects(shape, probe)) {
			outHitPos = desired;   // 既に埋まっている＝これ以上伸ばせない
			return true;
		}
	}
	return false;
}

// 形状どうしの交差判定ユーティリティ
#pragma once
#include "Collider.h"
#include "CollisionFunctions.h"

bool Intersects(const Shape& a, const Shape& b);

// 押し出し用：接触法線とめり込み量も返す。
// out.normal は「a を b から押し出す向き」。Mesh×Sphere/Capsule で有効、
// それ以外は検出のみ（out.hit だけ、depth=0）。
bool Intersects(const Shape& a, const Shape& b, Contact& out);

// 連続判定（CCD）：球を from → to へ進め、最初にメッシュへ接触した位置を返す。
// 1フレームの移動量が半径を超えると離散判定ではすり抜けるため、半径より細かい刻みで
// 分割して調べる。移動量が刻み幅以下なら false（離散判定だけで足りる）。
// outPos には最初に接触した時点の球の中心が入る。
bool SweepSphereVsMesh(const Shape& mesh, const Vector3& from, const Vector3& to,
    float radius, Vector3& outPos);
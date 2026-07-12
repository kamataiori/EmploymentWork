// 形状どうしの交差判定ユーティリティ
#pragma once
#include "Collider.h"
#include "CollisionFunctions.h"

bool Intersects(const Shape& a, const Shape& b);

// 押し出し用：接触法線とめり込み量も返す。
// out.normal は「a を b から押し出す向き」。Mesh×Sphere/Capsule で有効、
// それ以外は検出のみ（out.hit だけ、depth=0）。
bool Intersects(const Shape& a, const Shape& b, Contact& out);
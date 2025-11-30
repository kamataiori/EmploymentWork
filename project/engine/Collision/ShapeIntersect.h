// 形状どうしの交差判定ユーティリティ
#pragma once
#include "Collider.h"
#include "CollisionFunctions.h"

bool Intersects(const Shape& a, const Shape& b);
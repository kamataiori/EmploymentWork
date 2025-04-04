#pragma once
#include <DirectXMath.h>

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;

	// DirectX::XMFLOAT3 への変換演算子を追加
	operator DirectX::XMFLOAT3() const {
		return { x, y, z };
	}

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }

	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }

	// 符号反転演算子のオーバーロード
	Vector3 operator-() const { return Vector3{ -x, -y, -z }; }

	Vector3 operator*(const float other) const { return { x * other, y * other, z * other }; }

	Vector3 operator/(const float other) const { return { x / other, y / other, z / other }; }

	Vector3 operator+=(const Vector3& other) {
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}
};
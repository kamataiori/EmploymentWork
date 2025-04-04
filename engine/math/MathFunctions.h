#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3x3.h"
#include "Matrix4x4.h"
#include "math.h"
#include <fstream>
#include <sstream>
#include <Struct.h>
#include <Quaternion.h>

//単位行列の作成
Matrix4x4 MakeIdentity4x4();

//1. X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);

//2. Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian);

//3. Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian);

//拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

//平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4  Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

Vector3 Multiply(float scalar, Vector3& vec);

// アフィン変換行列計算関数
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rot, const Vector3& translate);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rot, const Vector3& translate);

//逆行列
Matrix4x4 Inverse(const Matrix4x4& matrix);

//透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

//正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

float Length(const Vector3& v);

Vector3 Normalize(const Vector3& v);

Vector3 Add(const Vector3& v1, const Vector3& v2);

Vector3 Cross(const Vector3& v1, const Vector3& v2);

Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

float LengthSq(const Vector3& v);

Vector3 Clamp(const Vector3& v, const Vector3& min, const Vector3& max);

float Dot(const Vector3& a, const Vector3& b);

float Clamp(float v, float min, float max);

// ヘルパー関数: 2点間の最小距離を計算
float DistanceSq(const Vector3& a, const Vector3& b);

// OBBの投影
float ProjectOBBOnAxis(const OBB& obb, const Vector3& axis);

Quaternion Slerp(const Quaternion& start, const Quaternion& end, float t);

Matrix4x4 transpose(const Matrix4x4& matrix);

// ベクトルを行列で変換する関数
Vector3 TransformCoord(const Vector3& vec, const Matrix4x4& mat);

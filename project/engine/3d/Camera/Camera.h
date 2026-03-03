#pragma once
#include "Transform.h"
#include "Matrix4x4.h"
#include "MathFunctions.h"

class Camera
{
public:
	/// <summary>
	/// デフォルトコンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

public:
	//------setter------//
	void SetRotate(const Vector3& rotate) {transform.rotate = rotate;};
	void SetTranslate(const Vector3& translate) {transform.translate = translate;}
	void SetFovY(float fovY_deg) {
		horizontalViewingAngle = fovY_deg * (3.1415926535f / 180.0f);
	}
	void SetAspectRatio(float aspectRatio) {aspect = aspectRatio;};
	void SetNearClip(float nearClip) {this->nearClip = nearClip;};
	void SetFarClip(float farClip) {this->farClip = farClip;};

	//------演出用加算API------
	void AddTranslate(const Vector3& offset);
	void AddRotate(const Vector3& offset);

	//------getter------//
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	//const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	Matrix4x4 GetViewMatrix() const;
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }

	float GetFovYRad() const { return horizontalViewingAngle; }
	float GetFovYDeg() const { return horizontalViewingAngle * (180.0f / 3.1415926535f); }


protected:

	//------ビュー行列関連データ------//
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;

	//------プロジェクション行列関連データ------//
	Matrix4x4 projectionMatrix;
	//水平方向視野角
	float horizontalViewingAngle;
	//アスペクト比
	float aspect;
	//ニアクリップ
	float nearClip;
	//ファークリップ距離
	float farClip;

	//合成行列
	Matrix4x4 viewProjectionMatrix;

	Vector3 position{};
	Vector3 rotation{};
};


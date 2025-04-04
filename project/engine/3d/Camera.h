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
	void SetFovY(float fovY) {horizontalViewingAngle = fovY;};
	void SetAspectRatio(float aspectRatio) {aspect = aspectRatio;};
	void SetNearClip(float nearClip) {this->nearClip = nearClip;};
	void SetFarClip(float farClip) {this->farClip = farClip;};

	//------getter------//
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }


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
};


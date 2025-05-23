#include "Camera.h"
#include <WinApp.h>

Camera::Camera()
    : transform({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} })
    , horizontalViewingAngle(0.45f)
    , aspect(float(WinApp::kClientWidth) / float(WinApp::kClientHeight))
    , nearClip(0.1f)
    , farClip(100.0f)
    , worldMatrix(MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
    , viewMatrix(Inverse(worldMatrix))
    , projectionMatrix(MakePerspectiveFovMatrix(horizontalViewingAngle, aspect, nearClip, farClip))
    , viewProjectionMatrix(Multiply(viewMatrix, projectionMatrix))
{}

void Camera::Update()
{
	//TransformからworldMatrixを作る
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	//cameraMatrixからviewMatrixを作る
	Matrix4x4 viewMatrix = Inverse(worldMatrix);
	//プロジェクション行列更新
	projectionMatrix = MakePerspectiveFovMatrix(horizontalViewingAngle, aspect, nearClip, farClip);
	//合成行列
	viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

    //ImGui::Begin("camera");
    ////ImGui::Text("cameraChange : SPACE");
    //ImGui::DragFloat3("translate", &transform.translate.x);
    //ImGui::DragFloat3("rotate", &transform.rotate.x);
    //ImGui::End();
}

Matrix4x4 Camera::GetViewMatrix() const
{
    Matrix4x4 rx = MakeRotateXMatrix(transform.rotate.x);
    Matrix4x4 ry = MakeRotateYMatrix(transform.rotate.y);
    Matrix4x4 rz = MakeRotateZMatrix(transform.rotate.z);
    Matrix4x4 rot = Multiply(rx, Multiply(ry, rz));

    Matrix4x4 trans = MakeTranslateMatrix(transform.translate);

    return Inverse(Multiply(rot, trans));
}

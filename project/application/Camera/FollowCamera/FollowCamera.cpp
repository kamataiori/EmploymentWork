#include "FollowCamera.h"
#include <cassert>

FollowCamera::FollowCamera(CharacterBase* target, float distance, float height, float horizontalOffset)
    : target_(target), distance_(distance), height_(height), horizontalOffset_(horizontalOffset) {
}

void FollowCamera::Update()
{
    assert(target_);

    const Vector3& targetPos = target_->GetTransform().translate;

    // プレイヤーの背後 + 左オフセット + 高さ
    Vector3 offset = {
        horizontalOffset_,  // ← X軸: 左右位置調整（-でプレイヤーを左寄りに）
        height_,            // ← Y軸: カメラの高さ
        distance_           // ← Z軸: カメラが後ろに下がる距離
    };

    // カメラ位置をプレイヤーからの相対オフセットで即設定（補間なし）
    transform.translate = targetPos - offset;

    // 視線方向も常にプレイヤーを向く（回転も即更新）
    Vector3 flatOffset = { 0.0f, height_, distance_ };  // 横ずれ除去
    Vector3 lookAtPos = targetPos - flatOffset;

    Vector3 toTarget = Normalize(targetPos - lookAtPos);
    transform.rotate.y = std::atan2(toTarget.x, toTarget.z);

    /*transform.rotate = {};
    transform.translate = { 0, 0, -10.0f };*/

    Camera::Update();
}

void FollowCamera::Debug()
{
#ifdef _DEBUG
    ImGui::Begin("FollowCamera");

    ImGui::SliderFloat("Distance (Z)", &distance_, 0.0f, 30.0f);
    ImGui::SliderFloat("Height (Y)", &height_, -10.0f, 20.0f);
    ImGui::SliderFloat("Horizontal Offset (X)", &horizontalOffset_, -10.0f, 10.0f);

    ImGui::End();
#endif
}

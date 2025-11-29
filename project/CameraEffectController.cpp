#include "CameraEffectController.h"

/// <summary>
/// カメラ演出の更新処理
/// 
/// 現状は
///   1. FOVズーム（CameraZoom）
///   2. シェイク（CameraShake）
/// の順に適用。
///
/// ※ 今後演出を追加したいときは、ここに Update を追加していくだけでよい
/// </summary>
void CameraEffectController::Update(Camera* camera, float deltaTime)
{
    if (!camera)
    {
        return;
    }

    // 1. FOV ズーム
    zoom_.Update(camera, deltaTime);

    // 2. シェイク
    shake_.Update(camera, deltaTime);
}

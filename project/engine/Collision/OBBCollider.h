#pragma once
#include "Collider.h"

class OBBCollider : public Collider {
public:
    OBB obb;

    OBBCollider(const OBB& box);

    bool Dispatch(Collider* other) override;

    void Draw() override;

    // 位置を設定する関数を追加
    Vector3 GetCenter() const;
    void SetPosition(const Vector3& position);

     Vector3 GetScale() const { return obb.size; }
    void SetScale(const Vector3& scale) { obb.size = scale; }
    
    void GetRotation(Vector3 orientations[3]) const {
        for (int i = 0; i < 3; ++i) {
            orientations[i] = obb.orientations[i];
        }
    }
    void SetRotation(const Vector3 orientations[3]) {
        for (int i = 0; i < 3; ++i) {
            obb.orientations[i] = orientations[i];
        }
    }


protected:
    bool Action(SphereCollider* other) override;
    bool Action(AABBCollider* other) override;
    bool Action(OBBCollider* other) override;
    bool Action(CapsuleCollider* other) override;
};

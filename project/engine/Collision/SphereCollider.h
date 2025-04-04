#pragma once
#include "Collider.h"

class SphereCollider : public Collider {
public:
    Sphere sphere;
    SphereCollider(const Sphere& s);

    bool Dispatch(Collider* other) override;

    void Draw() override;

    // 位置を設定する関数を追加
    Vector3 GetCenter() const;
    void SetPosition(const Vector3& position);

    // Radius (半径) Getter / Setter
    float GetRadius() const;
    void SetRadius(float radius);

    // Color Getter / Setter
    int GetColor() const;
    void SetColor(int color);

protected:
    bool Action(SphereCollider* other) override;
    bool Action(AABBCollider* other) override;
    bool Action(OBBCollider* other) override;
    bool Action(CapsuleCollider* other) override;
};

#pragma once
#include "Collider.h"

class AABBCollider : public Collider {
public:
    AABB aabb;

    AABBCollider(const AABB& box);

    bool Dispatch(Collider* other) override;

    void Draw() override;

protected:
    bool Action(SphereCollider* other) override;
    bool Action(AABBCollider* other) override;
    bool Action(OBBCollider* other) override;
    bool Action(CapsuleCollider* other) override;
};

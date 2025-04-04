#pragma once
#include "Collider.h"

class CapsuleCollider : public Collider {
public:
    Capsule capsule;

    CapsuleCollider(const Capsule& cap);

    bool Dispatch(Collider* other) override;

    void Draw() override;

protected:
    bool Action(SphereCollider* other) override;
    bool Action(AABBCollider* other) override;
    bool Action(OBBCollider* other) override;
    bool Action(CapsuleCollider* other) override;
};

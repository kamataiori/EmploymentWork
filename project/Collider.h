#pragma once
#include "CollisionFunctions.h"
#include "DrawLine.h"

class SphereCollider;
class AABBCollider;
class OBBCollider;
class CapsuleCollider;

class Collider {
public:
    
    virtual bool Dispatch(Collider* other) = 0;

    virtual void Draw() = 0;

    virtual void OnCollision();

public:
    virtual bool Action(SphereCollider* other) = 0;
    virtual bool Action(AABBCollider* other) = 0;
    virtual bool Action(OBBCollider* other) = 0;
    virtual bool Action(CapsuleCollider* other) = 0;

    // 種別IDを取得
    uint32_t GetTypeID() const { return typeID_; }
    // 種別IDを設定
    void SetTypeID(uint32_t typeID) { typeID_ = typeID; }

private:

    // 種別ID
    uint32_t typeID_ = 0u;
};
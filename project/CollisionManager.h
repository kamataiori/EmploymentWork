#pragma once
#include "Struct.h"
#include <list>
#include <algorithm>
#include "CollisionTypeIdDef.h"


class Collider;

class CollisionManager {
public:
    void RegisterCollider(Collider* collider);
    void UnregisterCollider(Collider* collider);
    void Reset();
    void CheckAllCollisions();

private:

    bool ShouldIgnoreCollision(CollisionTypeIdDef type1, CollisionTypeIdDef type2);

    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
        }
    };

private:
    std::list<Collider*> colliders;
};



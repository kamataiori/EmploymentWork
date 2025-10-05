#pragma once
#include "CharacterBase.h"
#include "SphereCollider.h"
#include "CollisionTypeIdDef.h"

#include "BehaviorTree.h"
#include "BTNodeEditor.h"

#include <memory>
#include <string>
#include <unordered_map>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


class Enemy : public CharacterBase, public SphereCollider
{
public:

    Enemy(BaseScene* baseScene_) : CharacterBase(baseScene_), SphereCollider(sphere) {}
   /* ~Enemy() override = default;*/

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollision();
    void SkinningDraw() override;
    void ParticleDraw() override;

    void SetTarget(CharacterBase* t){ bb_.target = t; }
    void SetMoveSpeed(float s){ bb_.moveSpeed = s; }

    // 可視化用に必要なら
    BTNode* RootBT(){ return tree_.Root(); }

private:
    void BuildBehaviorTree();
    float GetDeltaTime() const;
    void BuildBTView();
    void DrawBTView();

    // 追尾のみ
    float stopDistance_ = 0.2f;

private:
    BehaviorTree tree_;
    BTBlackboard bb_;

    // 可視化保持
    btvis::IdGen visIds_;
    std::unique_ptr<btvis::GraphView> visGraph_;
    std::unordered_map<const BTNode*, btvis::NodeBase*> visBind_;
    BTNode* nodeRoot_ = nullptr;
    BTNode* nodeChase_ = nullptr;
};


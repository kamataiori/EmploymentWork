#include "BehaviorTree.h"

// -------- BTSelector ----------
BTStatus BTSelector::Tick(BTBlackboard& bb, float dt) {
    if (children_.empty()) { Notify(BTStatus::Failure); return BTStatus::Failure; }
    for (; index_ < children_.size(); ++index_) {
        auto st = children_[index_]->Tick(bb, dt);
        if (st == BTStatus::Success) { index_ = 0; Notify(st); return st; }
        if (st == BTStatus::Running) { Notify(st); return st; }
        // Failureなら次の子へ
    }
    index_ = 0; Notify(BTStatus::Failure); return BTStatus::Failure;
}

// -------- BTSequence ----------
BTStatus BTSequence::Tick(BTBlackboard& bb, float dt) {
    if (children_.empty()) { Notify(BTStatus::Success); return BTStatus::Success; }
    for (; index_ < children_.size(); ++index_) {
        auto st = children_[index_]->Tick(bb, dt);
        if (st == BTStatus::Failure) { index_ = 0; Notify(st); return st; }
        if (st == BTStatus::Running) { Notify(st); return st; }
        // Successなら次の子へ
    }
    index_ = 0; Notify(BTStatus::Success); return BTStatus::Success;
}

// -------- BehaviorTree --------
void BehaviorTree::SetVisual(BTVisualCallback cb) {
    if (!root_) return;
    TraverseSet(root_.get(), cb);
}
void BehaviorTree::TraverseSet(BTNode* n, const BTVisualCallback& cb) {
    n->SetVisual(cb);
    if (auto s = dynamic_cast<BTSelector*>(n)) {
        for (auto& c : s->Children()) TraverseSet(c.get(), cb);
    }
    else if (auto q = dynamic_cast<BTSequence*>(n)) {
        for (auto& c : q->Children()) TraverseSet(c.get(), cb);
    }
}

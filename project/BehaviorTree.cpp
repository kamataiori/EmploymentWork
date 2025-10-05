#include "BehaviorTree.h"

BTStatus BTSelector::Tick(BTBlackboard& bb, float dt) {
    if (children_.empty()) { Notify(BTStatus::Failure); return BTStatus::Failure; }
    for (; idx_ < children_.size(); ++idx_) {
        auto st = children_[idx_]->Tick(bb, dt);
        if (st == BTStatus::Success) { idx_ = 0; Notify(st); return st; }
        if (st == BTStatus::Running) { Notify(st); return st; }
    }
    idx_ = 0; Notify(BTStatus::Failure); return BTStatus::Failure;
}

BTStatus BTSequence::Tick(BTBlackboard& bb, float dt) {
    if (children_.empty()) { Notify(BTStatus::Success); return BTStatus::Success; }
    for (; idx_ < children_.size(); ++idx_) {
        auto st = children_[idx_]->Tick(bb, dt);
        if (st == BTStatus::Failure) { idx_ = 0; Notify(st); return st; }
        if (st == BTStatus::Running) { Notify(st); return st; }
    }
    idx_ = 0; Notify(BTStatus::Success); return BTStatus::Success;
}

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

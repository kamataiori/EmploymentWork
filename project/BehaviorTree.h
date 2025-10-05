#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <string>

enum class BTStatus { Running, Success, Failure };
class BTNode;
using BTVisualCallback = std::function<void(const BTNode*, BTStatus)>;

// BlackBoard（後で項目を増やせる）
class CharacterBase;
struct BTBlackboard {
    CharacterBase* self = nullptr;
    CharacterBase* target = nullptr;
    float moveSpeed = 8.0f;
    float distToTarget = 9999.0f;
};

// ノード基底
class BTNode {
public:
    explicit BTNode(const std::string& name) : name_(name) {}
    virtual ~BTNode() = default;
    virtual BTStatus Tick(BTBlackboard& bb, float dt) = 0;
    virtual void Reset() {}
    const std::string& Name() const { return name_; }
    void SetVisual(BTVisualCallback cb) { visual_ = std::move(cb); }

protected:

    void Notify(BTStatus s) { if (visual_) visual_(this, s); }

private:
    std::string name_;
    BTVisualCallback visual_{};
};

// Composite
class BTSelector : public BTNode {
public:
    using BTNode::BTNode;
    void Add(std::unique_ptr<BTNode> n) { children_.emplace_back(std::move(n)); }
    BTStatus Tick(BTBlackboard& bb, float dt) override;
    void Reset() override { idx_ = 0; for (auto& c : children_) c->Reset(); }
    const std::vector<std::unique_ptr<BTNode>>& Children() const { return children_; }

private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t idx_ = 0;
};

class BTSequence : public BTNode {
public:
    using BTNode::BTNode;
    void Add(std::unique_ptr<BTNode> n) { children_.emplace_back(std::move(n)); }
    BTStatus Tick(BTBlackboard& bb, float dt) override;
    void Reset() override { idx_ = 0; for (auto& c : children_) c->Reset(); }
    const std::vector<std::unique_ptr<BTNode>>& Children() const { return children_; }

private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t idx_ = 0;
};

// Leaf
class BTAction : public BTNode {
public:
    using Fn = std::function<BTStatus(BTBlackboard&, float)>;
    BTAction(const std::string& n, Fn fn) :BTNode(n), fn_(std::move(fn)) {}
    BTStatus Tick(BTBlackboard& bb, float dt) override { auto s = fn_(bb, dt); Notify(s); return s; }

private:
    Fn fn_;
};

// Tree
class BehaviorTree {
public:
    void SetRoot(std::unique_ptr<BTNode> r) { root_ = std::move(r); }
    BTStatus Tick(BTBlackboard& bb, float dt) { return root_ ? root_->Tick(bb, dt) : BTStatus::Failure; }
    void SetVisual(BTVisualCallback cb);
    BTNode* Root() const { return root_.get(); }

private:
    void TraverseSet(BTNode* n, const BTVisualCallback& cb);
    std::unique_ptr<BTNode> root_;
};


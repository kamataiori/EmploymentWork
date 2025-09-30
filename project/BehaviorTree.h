#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <string>

// ===== 可視化コールバック（任意） =====
enum class BTStatus { Running, Success, Failure };
class BTNode;
using BTVisualCallback = std::function<void(const BTNode*, BTStatus)>;

// 前方宣言（循環参照回避）
class CharacterBase;

// ===== ブラックボード（必要に応じて拡張） =====
struct BTBlackboard {
    CharacterBase* self = nullptr;
    CharacterBase* target = nullptr;

    // パラメータ（簡易）
    float moveSpeed = 8.0f;      // 追跡速度
    float attackRange = 2.5f;    // 近接射程
    float attackCooldown = 1.2f; // 攻撃CD(秒)

    // ランタイム
    float attackCDTimer = 0.0f;  // 残CD
    float distToTarget = 9999.0f;

    // ゲーム側の攻撃呼び出し
    std::function<void()> DoAttack; // 成功時に呼ぶ
};

// ===== ノード基底 =====
class BTNode {
public:
    explicit BTNode(const std::string& name) : name_(name) {}
    virtual ~BTNode() = default;

    const std::string& Name() const { return name_; }
    virtual BTStatus Tick(BTBlackboard& bb, float dt) = 0;
    virtual void Reset() {}

    // 可視化用（任意）
    void SetVisual(BTVisualCallback cb) { visual_ = std::move(cb); }
protected:
    void Notify(BTStatus s) { if (visual_) visual_(this, s); }
private:
    std::string name_;
    BTVisualCallback visual_{};
};

// ===== Composite: Selector / Sequence =====
class BTSelector : public BTNode {
public:
    using BTNode::BTNode;
    void Add(std::unique_ptr<BTNode> n) { children_.emplace_back(std::move(n)); }
    BTStatus Tick(BTBlackboard& bb, float dt) override;
    void Reset() override { index_ = 0; for (auto& c : children_) c->Reset(); }
    const std::vector<std::unique_ptr<BTNode>>& Children() const { return children_; }
private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t index_ = 0;
};

class BTSequence : public BTNode {
public:
    using BTNode::BTNode;
    void Add(std::unique_ptr<BTNode> n) { children_.emplace_back(std::move(n)); }
    BTStatus Tick(BTBlackboard& bb, float dt) override;
    void Reset() override { index_ = 0; for (auto& c : children_) c->Reset(); }
    const std::vector<std::unique_ptr<BTNode>>& Children() const { return children_; }
private:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t index_ = 0;
};

// ===== Leaf: 条件 / 行動 =====
class BTCondition : public BTNode {
public:
    using Fn = std::function<bool(const BTBlackboard&)>;
    BTCondition(const std::string& n, Fn fn) : BTNode(n), fn_(std::move(fn)) {}
    BTStatus Tick(BTBlackboard& bb, float /*dt*/) override {
        auto ok = fn_(bb); auto st = ok ? BTStatus::Success : BTStatus::Failure; Notify(st); return st;
    }
private:
    Fn fn_;
};

class BTAction : public BTNode {
public:
    using Fn = std::function<BTStatus(BTBlackboard&, float)>;
    BTAction(const std::string& n, Fn fn) : BTNode(n), fn_(std::move(fn)) {}
    BTStatus Tick(BTBlackboard& bb, float dt) override {
        auto st = fn_(bb, dt); Notify(st); return st;
    }
private:
    Fn fn_;
};

// ===== ツリー管理 =====
class BehaviorTree {
public:
    void SetRoot(std::unique_ptr<BTNode> r) { root_ = std::move(r); }
    void SetVisual(BTVisualCallback cb); // 全ノードへ設定
    BTStatus Tick(BTBlackboard& bb, float dt) { return root_ ? root_->Tick(bb, dt) : BTStatus::Failure; }
    BTNode* Root() const { return root_.get(); }

private:
    void TraverseSet(BTNode* node, const BTVisualCallback& cb);
    std::unique_ptr<BTNode> root_;
};

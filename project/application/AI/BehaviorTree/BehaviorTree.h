#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

//======================================================
// BehaviorTree (Minimal)
//------------------------------------------------------
// ・ゲームAIでよく使われる「行動木」の最小実装
// ・Enemy専用ではなく、将来はNPC/味方/ボスにも使える汎用設計
// ・ノードは Success / Failure / Running を返す
//
// - 汎用の最小BT（Condition / Action / Sequence / Selector）
// 用語イメージ（UE5 Niagara/BTに近い）
//   Condition : 条件チェック（true/false）
//   Action    : 実際の処理（移動・攻撃など）
//   Sequence  : 上から順に実行（全部成功でSuccess）
//   Selector  : 上から順に実行（どれか成功でSuccess）
// - 今後 ImGui Node Editor に繋げやすいように name を保持
//======================================================
namespace BT {

	//==================================================
	// ノードの実行結果
	// Success : このノードは成功
	// Failure : このノードは失敗
	// Running : 処理継続中（次フレームもこのノードを実行）
	//==================================================
	enum class Status {
		Success,
		Failure,
		Running
	};

	//==================================================
	// Context
	//--------------------------------------------------
	// ・BT実行時にノード間で共有される情報
	// ・owner  : 実行主体（Enemy* を void* で保持）
	// ・dt     : デルタタイム
	// ・target : 探索結果（PlayerのTransform等）
	// ※ void* を使っている理由
	//   → include依存を減らし、BTを汎用に保つため
	//==================================================
	struct Context {
		void* owner = nullptr;
		float dt = 0.0f;

		// 「見つけたターゲット」を次ノードへ渡すために持っておく
		// 今回は const Transform* を入れる
		const void* target = nullptr;
	};

	//==================================================
	// Node（全ノードの基底クラス）
	//==================================================
	class Node {
	public:
		virtual ~Node() = default;
		// 毎フレーム呼ばれる
		virtual Status Tick(Context& ctx) = 0;

		// デバッグ・可視化用ノード名
		const std::string& GetName() const { return name_; }
		void SetName(const std::string& n) { name_ = n; }

	protected:
		std::string name_ = "Node";
	};

	using NodePtr = std::unique_ptr<Node>;

	//==================================================
	// Composite
	//--------------------------------------------------
	// ・子ノードを複数持つノードの基底
	// ・Sequence / Selector がこれを継承
	//==================================================
	class Composite : public Node {
	public:
		void AddChild(NodePtr child) { children_.push_back(std::move(child)); }
		const std::vector<NodePtr>& GetChildren() const { return children_; }

	protected:
		std::vector<NodePtr> children_;
	};

	//==================================================
	// Sequence
	// - 全て成功でSuccess
	// - 失敗が1つでも出たらFailure
	// - Running が出たらその場で Running
	// 例
	//   FindTarget → IsFar → Chase
	//==================================================
	class Sequence : public Composite {
	public:
		Sequence() { name_ = "Sequence"; }

		Status Tick(Context& ctx) override {
			for (auto& c : children_) {
				Status s = c->Tick(ctx);
				if (s == Status::Failure) { return Status::Failure; }
				if (s == Status::Running) { return Status::Running; }
			}
			return Status::Success;
		}
	};

	//==================================================
	// Selector
	// - 1つでも成功でSuccess
	// - 全て失敗でFailure
	// - Running が出たらその場で Running
	// 例
	//   攻撃A → 攻撃B → Idle
	//==================================================
	class Selector : public Composite {
	public:
		Selector() { name_ = "Selector"; }

		Status Tick(Context& ctx) override {
			for (auto& c : children_) {
				Status s = c->Tick(ctx);
				if (s == Status::Success) { return Status::Success; }
				if (s == Status::Running) { return Status::Running; }
			}
			return Status::Failure;
		}
	};

	//==================================================
	// Condition
	//--------------------------------------------------
	// ・true / false を返す条件ノード
	// ・内部では bool を Status に変換
	//
	// 例
	//   IsPlayerFound
	//   IsPlayerFar
	//==================================================
	class Condition : public Node {
	public:
		using Func = std::function<bool(Context&)>;
		explicit Condition(Func f, std::string name = "Condition")
			: func_(std::move(f)) {
			name_ = std::move(name);
		}

		Status Tick(Context& ctx) override {
			return func_(ctx) ? Status::Success : Status::Failure;
		}

	private:
		Func func_;
	};

	//==================================================
	// Action
	//--------------------------------------------------
	// ・実際の処理を行うノード
	// ・Success / Failure / Running を返せる
	//
	// 例
	//   Chase
	//   Attack
	//   Wait
	//==================================================
	class Action : public Node {
	public:
		using Func = std::function<Status(Context&)>;
		explicit Action(Func f, std::string name = "Action")
			: func_(std::move(f)) {
			name_ = std::move(name);
		}

		Status Tick(Context& ctx) override {
			return func_(ctx);
		}

	private:
		Func func_;
	};

	//==================================================
	// Tree
	//--------------------------------------------------
	// ・BehaviorTree の本体
	// ・ルートノードを1つだけ持つ
	//==================================================
	class Tree {
	public:
		// ルートノードを設定
		void SetRoot(NodePtr root) {
			root_ = std::move(root);
		}

		// ルートノード取得（デバッグ・可視化用）
		Node* GetRoot() const {
			return root_.get();
		}

		// 毎フレーム呼ばれるBTの更新処理
		Status Tick(Context& ctx) {
			if (!root_) {
				// ルートが無い＝何も行動できない
				return Status::Failure;
			}
			return root_->Tick(ctx);
		}

	private:
		NodePtr root_; // BTの開始地点
	};

} // namespace BT

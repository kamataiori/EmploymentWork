// BTNodeEditor.h — シンプルなBT可視化の基盤実装（imgui-node-editor）
// 使い方:
//   1) プロジェクトで imgui と imgui-node-editor をリンク
//   2) GraphView を作成し、任意の NodeBase 派生を追加
//   3) 実行時に BT の状態変化に応じて NodeBase::SetExecState を呼ぶ
//   4) フレーム毎に GraphView::Draw() を呼ぶ
//
//   ※このヘッダは最小構成、プロダクションではセーブ/ロード、コンテキストメニュー等を拡張する必要性あり

#pragma once
#include <externals/imgui/imgui.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

// imgui-node-editor
// インクルードパスは環境に合わせて調整してください。
#include <imgui_node_editor.h>
//#include <externals/imgui/imgui_node_editor.cpp>
namespace ed = ax::NodeEditor;

namespace btvis {

    // 実行状態（BTの現在位置を色で可視化）
    enum class ExecState : uint8_t {
        Idle,
        Running,
        Succeeded,
        Failed
    };

    // 単純なID生成器（セーブ/ロード対応は各自拡張）
    struct IdGen {
        int nextNode = 1;
        int nextPin = 1;
        int nextLink = 1;

        ed::NodeId NewNode() { return ed::NodeId(nextNode++); }
        ed::PinId  NewPin() { return ed::PinId(nextPin++); }
        ed::LinkId NewLink() { return ed::LinkId(nextLink++); }
    };

    // ピンの方向
    enum class PinDir : uint8_t { In, Out };

    struct PinDesc {
        ed::PinId id;
        PinDir dir;
        std::string label;
    };

    // ノード基底
    class NodeBase {
    public:
        explicit NodeBase(const std::string& title)
            : id_(0), title_(title) {
        }

        virtual ~NodeBase() = default;

        void AssignId(ed::NodeId id) { id_ = id; }
        ed::NodeId Id() const { return id_; }

        void AddInputPin(const std::string& label, ed::PinId pid) {
            inputs_.push_back(PinDesc{ pid, PinDir::In, label });
        }
        void AddOutputPin(const std::string& label, ed::PinId pid) {
            outputs_.push_back(PinDesc{ pid, PinDir::Out, label });
        }

        const std::string& Title() const { return title_; }

        ExecState GetExecState() const { return state_; }
        void SetExecState(ExecState s) { state_ = s; }

        // 実行可視化の色設定（必要に応じて調整）
        virtual ImColor StateColor() const {
            switch (state_) {
            case ExecState::Running:   return ImColor(0, 180, 255);
            case ExecState::Succeeded: return ImColor(80, 220, 120);
            case ExecState::Failed:    return ImColor(220, 80, 80);
            default:                   return ImColor(80, 80, 80);
            }
        }

        // ノード内部の描画（派生で中身を差し替え可）
        virtual void DrawContent() {
            ImGui::TextUnformatted(title_.c_str());
        }

        // ノード全体の描画
        virtual void DrawNode() {
            ed::BeginNode(id_);
            // ヘッダ風の上部
            {
                ImGui::PushStyleColor(ImGuiCol_Text, StateColor().Value);
                ImGui::Text("%s", title_.c_str());
                ImGui::PopStyleColor();
            }

            // 入力ピン
            for (auto& p : inputs_) {
                ed::BeginPin(p.id, ed::PinKind::Input);
                ImGui::Text("◀ %s", p.label.c_str());
                ed::EndPin();
            }

            // コンテンツ
            DrawContent();

            // 出力ピン
            for (auto& p : outputs_) {
                ed::BeginPin(p.id, ed::PinKind::Output);
                ImGui::Text("%s ▶", p.label.c_str());
                ed::EndPin();
            }

            // 状態に応じた枠の強調
            {
                ImColor col = StateColor();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();

                // 余白（パディング）
                const ImVec2 pad(6.0f, 6.0f);
                ImVec2 a(min.x - pad.x, min.y - pad.y);
                ImVec2 b(max.x + pad.x, max.y + pad.y);

                ed::Suspend();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRect(a, b, (ImU32)col, 6.0f, 0, 2.0f); // 色は ImU32
                ed::Resume();
            }

            ed::EndNode();
        }

    protected:
        ed::NodeId id_;
        std::string title_;
        std::vector<PinDesc> inputs_;
        std::vector<PinDesc> outputs_;
        ExecState state_ = ExecState::Idle;
    };

    // 代表ノード型（必要に応じて見た目を差別化）
    class CompositeNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override {
            ImGui::Separator();
            ImGui::TextDisabled("Composite");
        }
    };

    class ActionNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override {
            ImGui::Separator();
            ImGui::TextDisabled("Action");
        }
    };

    class DecoratorNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override {
            ImGui::Separator();
            ImGui::TextDisabled("Decorator");
        }
    };

    struct LinkDesc {
        ed::LinkId id;
        ed::PinId a;
        ed::PinId b;
        ImColor color = ImColor(160, 160, 160);
    };

    // グラフ全体
    class GraphView {
    public:
        explicit GraphView(IdGen* idgen)
            : idgen_(idgen) {
            ed::Config cfg;
            cfg.SettingsFile = nullptr; // 外部ini保存を無効（必要ならパスを与える）
            ctx_ = ed::CreateEditor(&cfg);
        }

        ~GraphView() {
            if (ctx_) ed::DestroyEditor(ctx_);
        }

        ed::EditorContext* Ctx() const { return ctx_; }

        template<class T, class... Args>
        T* AddNode(const std::string& title, Args&&... args) {
            auto n = std::make_unique<T>(title, std::forward<Args>(args)...);
            n->AssignId(idgen_->NewNode());
            T* raw = n.get();
            nodes_.push_back(std::move(n));
            return raw;
        }

        // Pin追加ヘルパ
        ed::PinId AddInputPin(NodeBase* n, const std::string& label) {
            auto pid = idgen_->NewPin();
            n->AddInputPin(label, pid);
            return pid;
        }
        ed::PinId AddOutputPin(NodeBase* n, const std::string& label) {
            auto pid = idgen_->NewPin();
            n->AddOutputPin(label, pid);
            return pid;
        }

        // リンク作成
        LinkDesc* AddLink(ed::PinId a, ed::PinId b, ImColor color = ImColor(160, 160, 160)) {
            LinkDesc l{ idgen_->NewLink(), a, b, color };
            links_.push_back(l);
            return &links_.back();
        }

        // 実行中のBTノードポインタと可視ノードを紐づけ（任意管理）
        void BindRuntimePtr(const void* runtimeNodePtr, NodeBase* view) {
            runtime2view_[runtimeNodePtr] = view;
        }

        // 実行状態を更新（BT実行側からコール）
        void SetExecStateByPtr(const void* runtimeNodePtr, ExecState s) {
            auto it = runtime2view_.find(runtimeNodePtr);
            if (it != runtime2view_.end()) it->second->SetExecState(s);
        }

        // 全体描画
        void Draw(const char* windowTitle = "BT Graph") {
            ed::SetCurrentEditor(ctx_);
            ImGui::Begin(windowTitle);
            ed::Begin("BTNodeEditor");

            // ノード
            for (auto& n : nodes_) {
                n->DrawNode();
            }

            // リンク
            for (auto& l : links_) {
                ed::Link(l.id, l.a, l.b, l.color, 2.0f);
            }

            // ユーザ操作での新規リンク作成
            {
                ed::PinId start, end;
                if (ed::BeginCreate()) {
                    if (ed::QueryNewLink(&start, &end)) {
                        if (start && end && start != end) {
                            if (ed::AcceptNewItem()) {
                                links_.push_back({ idgen_->NewLink(), start, end, ImColor(200,200,200) });
                            }
                        }
                    }
                }
                ed::EndCreate();
            }

            // 既存リンク削除
            {
                ed::LinkId lid{};
                if (ed::BeginDelete()) {
                    while (ed::QueryDeletedLink(&lid)) {
                        if (ed::AcceptDeletedItem()) {
                            EraseLink(lid);
                        }
                    }
                }
                ed::EndDelete();
            }

            ed::End();
            ImGui::End();
            ed::SetCurrentEditor(nullptr);
        }

        // レイアウト（初回のみなどで呼び出し）
        void AutoLayoutGrid(float xStep = 280.0f, float yStep = 120.0f) {
            // 単純なグリッド配置（左から右へ）
            ImVec2 pos(0, 0);
            for (size_t i = 0; i < nodes_.size(); ++i) {
                ed::SetNodePosition(nodes_[i]->Id(), pos);
                pos.x += xStep;
                if ((i % 4) == 3) { pos.x = 0; pos.y += yStep; }
            }
        }

        // 便利: すべてのノード状態を Idle に戻す
        void ResetExecStates() {
            for (auto& n : nodes_) n->SetExecState(ExecState::Idle);
        }

    private:
        void EraseLink(ed::LinkId lid) {
            for (size_t i = 0; i < links_.size(); ++i) if (links_[i].id == lid) {
                links_.erase(links_.begin() + i); return;
            }
        }

    private:
        ed::EditorContext* ctx_ = nullptr;
        IdGen* idgen_ = nullptr;
        std::vector<std::unique_ptr<NodeBase>> nodes_;
        std::vector<LinkDesc> links_;
        std::unordered_map<const void*, NodeBase*> runtime2view_;
    };

    // ------------------------------------------------------------
    // 便利関数: 3系統（Defense/Attack/Move）を手早く作る
    // ------------------------------------------------------------
    struct SimplePacks {
        CompositeNodeView* defense = nullptr;
        CompositeNodeView* attack = nullptr;
        CompositeNodeView* move = nullptr;
    };

    inline SimplePacks BuildSimpleRoot(GraphView& g, IdGen& ids) {
        SimplePacks p;

        p.defense = g.AddNode<CompositeNodeView>("DefensePack");
        p.attack = g.AddNode<CompositeNodeView>("AttackPack");
        p.move = g.AddNode<CompositeNodeView>("MovePack");

        // 入出力ピンを適当に用意（上位Selectorを仮定）
        auto dIn = g.AddInputPin(p.defense, "In");
        auto dOut = g.AddOutputPin(p.defense, "Out");

        auto aIn = g.AddInputPin(p.attack, "In");
        auto aOut = g.AddOutputPin(p.attack, "Out");

        auto mIn = g.AddInputPin(p.move, "In");
        auto mOut = g.AddOutputPin(p.move, "Out");

        // 単純に Defense→Attack→Move の直列例（実際はSelectorで分岐）
        g.AddLink(dOut, aIn);
        g.AddLink(aOut, mIn);

        return p;
    }

    // ------------------------------------------------------------
    // 例：個別アクションを AttackPack にぶら下げるヘルパ
    // ------------------------------------------------------------
    inline void AttachAttackLeaves(GraphView& g, IdGen& /*ids*/, CompositeNodeView* attackPack) {
        auto proj = g.AddNode<ActionNodeView>("ProjectileShot");
        auto flam = g.AddNode<ActionNodeView>("FlameBurst");
        auto mele = g.AddNode<ActionNodeView>("MeleeSlash");

        auto aOut = g.AddOutputPin(attackPack, "Out"); // AttackPack からの出力を増設

        auto pIn = g.AddInputPin(proj, "In");
        auto pOut = g.AddOutputPin(proj, "Out");

        auto fIn = g.AddInputPin(flam, "In");
        auto fOut = g.AddOutputPin(flam, "Out");

        auto mIn = g.AddInputPin(mele, "In");
        auto mOut = g.AddOutputPin(mele, "Out");

        // 例として AttackPack → 各アクション → (どこかへ)
        g.AddLink(aOut, pIn);
        g.AddLink(aOut, fIn);
        g.AddLink(aOut, mIn);
    }

    // ------------------------------------------------------------
    // 使用例（アプリ側）
    // ------------------------------------------------------------
    /*
    // 初期化
    static btvis::IdGen s_ids;
    static std::unique_ptr<btvis::GraphView> s_graph;

    void InitBTEditor() {
        s_graph = std::make_unique<btvis::GraphView>(&s_ids);
        auto packs = btvis::BuildSimpleRoot(*s_graph, s_ids);
        btvis::AttachAttackLeaves(*s_graph, s_ids, packs.attack);
        s_graph->AutoLayoutGrid();
    }

    // フレーム描画
    void DrawBTEditor() {
        s_graph->Draw("Enemy BT");
    }

    // 実行状態の更新（ゲームロジック側）
    void OnBTNodeStateChanged(const void* runtimePtr, btvis::ExecState s) {
        s_graph->SetExecStateByPtr(runtimePtr, s);
    }
    */

} // namespace btvis

#pragma once
#include <imgui.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

// imgui-node-editor
#include <imgui_node_editor.h>
namespace ed = ax::NodeEditor;

namespace btvis {

    // 実行状態（色分け用）
    enum class ExecState : uint8_t { Idle, Running, Succeeded, Failed };

    // ID生成
    struct IdGen {
        int nextNode = 1, nextPin = 1, nextLink = 1;
        ed::NodeId NewNode() { return ed::NodeId(nextNode++); }
        ed::PinId  NewPin() { return ed::PinId(nextPin++); }
        ed::LinkId NewLink() { return ed::LinkId(nextLink++); }
    };

    // ピン
    enum class PinDir : uint8_t { In, Out };
    struct PinDesc { ed::PinId id; PinDir dir; std::string label; };

    // ノード基底ビュー
    class NodeBase {
    public:
        explicit NodeBase(const std::string& title) : id_(0), title_(title) {}
        virtual ~NodeBase() = default;

        void AssignId(ed::NodeId id) { id_ = id; }
        ed::NodeId Id() const { return id_; }

        void AddInputPin(const std::string& label, ed::PinId pid) { inputs_.push_back({ pid,PinDir::In ,label }); }
        void AddOutputPin(const std::string& label, ed::PinId pid) { outputs_.push_back({ pid,PinDir::Out,label }); }

        ExecState GetExecState() const { return state_; }
        void SetExecState(ExecState s) { state_ = s; }

        virtual ImColor StateColor() const {
            switch (state_) {
            case ExecState::Running: return ImColor(0, 180, 255);
            case ExecState::Succeeded: return ImColor(80, 220, 120);
            case ExecState::Failed: return ImColor(220, 80, 80);
            default: return ImColor(80, 80, 80);
            }
        }

        // 内容（派生で上書き可）
        virtual void DrawContent() {
            ImGui::TextUnformatted(title_.c_str());
        }

        // ノード描画
        virtual void DrawNode() {
            ed::BeginNode(id_);

            // ヘッダ
            ImGui::PushStyleColor(ImGuiCol_Text, StateColor().Value);   // ImVec4
            ImGui::Text("%s", title_.c_str());
            ImGui::PopStyleColor();

            // 入力ピン
            for (auto& p : inputs_) {
                ed::BeginPin(p.id, ed::PinKind::Input);
                ImGui::Text("◀ %s", p.label.c_str());
                ed::EndPin();
            }

            // 中身
            DrawContent();

            // 出力ピン
            for (auto& p : outputs_) {
                ed::BeginPin(p.id, ed::PinKind::Output);
                ImGui::Text("%s ▶", p.label.c_str());
                ed::EndPin();
            }

            // 状態枠（ImVec2演算子を使わない）
            {
                ImColor col = StateColor();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                const ImVec2 pad(6.0f, 6.0f);
                ImVec2 a(min.x - pad.x, min.y - pad.y);
                ImVec2 b(max.x + pad.x, max.y + pad.y);

                ed::Suspend();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRect(a, b, (ImU32)col, 6.0f, 0, 2.0f);
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

    // 代表ビュー型
    class CompositeNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override { ImGui::Separator(); ImGui::TextDisabled("Composite"); }
    };
    class ActionNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override { ImGui::Separator(); ImGui::TextDisabled("Action"); }
    };
    class DecoratorNodeView : public NodeBase {
    public:
        using NodeBase::NodeBase;
        void DrawContent() override { ImGui::Separator(); ImGui::TextDisabled("Decorator"); }
    };

    struct LinkDesc { ed::LinkId id; ed::PinId a, b; ImColor color = ImColor(160, 160, 160); };

    // グラフ管理
    class GraphView {
    public:
        explicit GraphView(IdGen* idgen) : idgen_(idgen) {
            ed::Config cfg;
            cfg.SettingsFile = nullptr; // 外部iniを使わない（必要ならパスを与える）
            ctx_ = ed::CreateEditor(&cfg);
        }
        ~GraphView() {
            if (ctx_) ed::DestroyEditor(ctx_);
        }

        template<class T, class... Args>
        T* AddNode(const std::string& title, Args&&... args) {
            auto n = std::make_unique<T>(title, std::forward<Args>(args)...);
            n->AssignId(idgen_->NewNode());
            T* raw = n.get();
            nodes_.push_back(std::move(n));
            // 追加直後にレイアウトしたい場合が多いので予約だけ入れておく（任意）
            pending_layout_ = true;
            return raw;
        }

        // ピン生成
        ed::PinId AddInputPin(NodeBase* n, const std::string& label) { auto id = idgen_->NewPin(); n->AddInputPin(label, id); return id; }
        ed::PinId AddOutputPin(NodeBase* n, const std::string& label) { auto id = idgen_->NewPin(); n->AddOutputPin(label, id); return id; }

        // リンク生成
        LinkDesc* AddLink(ed::PinId a, ed::PinId b, ImColor col = ImColor(160, 160, 160)) {
            links_.push_back({ idgen_->NewLink(), a, b, col });
            return &links_.back();
        }

        // ランタイムBTノード ⇔ 表示ノードの紐付け
        void BindRuntimePtr(const void* runtimeNodePtr, NodeBase* view) {
            runtime2view_[runtimeNodePtr] = view;
        }
        void SetExecStateByPtr(const void* runtimeNodePtr, ExecState s) {
            auto it = runtime2view_.find(runtimeNodePtr);
            if (it != runtime2view_.end()) it->second->SetExecState(s);
        }

        // ---- ここがポイント：次の Draw でレイアウトを実行する予約API ----
        void RequestAutoLayout(float xStep = 280.0f, float yStep = 120.0f) {
            pending_layout_ = true; pending_xstep_ = xStep; pending_ystep_ = yStep;
        }
        // 互換：以前の名前でも呼べるように
        void AutoLayoutGrid(float xStep = 280.0f, float yStep = 120.0f) {
            RequestAutoLayout(xStep, yStep);
        }

        // メイン描画
        void Draw(const char* windowTitle = "BT Graph") {
            ed::SetCurrentEditor(ctx_);

            // 初回だけ位置/サイズを決める（画面外保存対策）
            ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(640, 420), ImGuiCond_FirstUseEver);

            ImGui::Begin(windowTitle, nullptr, ImGuiWindowFlags_NoCollapse);
            ed::Begin("BTNodeEditor");

            // ノード描画
            for (auto& n : nodes_) n->DrawNode();

            // リンク描画
            for (auto& l : links_) ed::Link(l.id, l.a, l.b, l.color, 2.0f);

            // 追加/削除操作は省略（今まで通り）

            // 遅延レイアウト予約があればここで実行（描画中なので安全）
            if (pending_layout_) {
                ImVec2 pos(0, 0);
                for (size_t i = 0; i < nodes_.size(); ++i) {
                    ed::SetNodePosition(nodes_[i]->Id(), pos);
                    pos.x += pending_xstep_;
                    if ((i % 4) == 3) { pos.x = 0; pos.y += pending_ystep_; }
                }
                pending_layout_ = false;
                need_navigate_ = true; // レイアウト後は内容へカメラ移動
            }

            // 初回 or レイアウト後に1度だけ画面内へ寄せる
            if (need_navigate_) {
                ed::NavigateToContent();
                need_navigate_ = false;
            }

            ed::End();
            ImGui::End();
            ed::SetCurrentEditor(nullptr);
        }

        //void RequestAutoLayout(float xs = 280.f, float ys = 120.f) {
        //    pending_layout_ = true; pending_xstep_ = xs; pending_ystep_ = ys;
        //}
       //oid AutoLayoutGrid(float xs = 280.f, float ys = 120.f) { RequestAutoLayout(xs, ys); }

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

        // 遅延レイアウト用フラグと設定
        bool  pending_layout_ = false;
        float pending_xstep_ = 280.f;
        float pending_ystep_ = 120.f;
        bool  need_navigate_ = true;
    };

    //テンプレ便利関数
    struct SimplePacks { CompositeNodeView* defense = nullptr; CompositeNodeView* attack = nullptr; CompositeNodeView* move = nullptr; };

    inline SimplePacks BuildSimpleRoot(GraphView& g, IdGen&) {
        SimplePacks p;
        p.defense = g.AddNode<CompositeNodeView>("DefensePack");
        p.attack = g.AddNode<CompositeNodeView>("AttackPack");
        p.move = g.AddNode<CompositeNodeView>("MovePack");
        auto dIn = g.AddInputPin(p.defense, "In"); auto dOut = g.AddOutputPin(p.defense, "Out");
        auto aIn = g.AddInputPin(p.attack, "In"); auto aOut = g.AddOutputPin(p.attack, "Out");
        auto mIn = g.AddInputPin(p.move, "In"); auto mOut = g.AddOutputPin(p.move, "Out");
        g.AddLink(dOut, aIn); g.AddLink(aOut, mIn);
        return p;
    }

    inline void AttachAttackLeaves(GraphView& g, IdGen&, CompositeNodeView* attackPack) {
        auto proj = g.AddNode<ActionNodeView>("ProjectileShot");
        auto flam = g.AddNode<ActionNodeView>("FlameBurst");
        auto mele = g.AddNode<ActionNodeView>("MeleeSlash");
        auto aOut = g.AddOutputPin(attackPack, "Out");
        auto pIn = g.AddInputPin(proj, "In"); auto fIn = g.AddInputPin(flam, "In"); auto mIn = g.AddInputPin(mele, "In");
        g.AddLink(aOut, pIn); g.AddLink(aOut, fIn); g.AddLink(aOut, mIn);
    }

} // namespace btvis

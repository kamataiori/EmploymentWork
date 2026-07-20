#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include "engine/TimeManager.h"
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

//======================================================
// MissionBanner（バトル開始前に一文を流すUI）
//------------------------------------------------------
// 「敵を6体倒そう」のような目的を、画面右から左へ流して見せる。
// 出しっぱなしではなく Show() を呼んだときだけ1回流れる。
//
// 動きは 入る→止まる→抜ける の3段。ただ等速で流すと読み切れないので、
// 真ん中で一度止めて読ませてから左へ抜けさせる（全体としては右→左の流れ）。
//
// 文言は画像1枚ごとに用意し、Show(index) で選ぶ。複数の文言を1つの
// UIElement に持たせているのは、同時に2つ流れることが無いため。
//======================================================
class MissionBanner : public UIElement {
public:
    // 文言の種類。CreateDesc::texPaths はこの並び順で渡すこと。
    enum Message {
        Swarm = 0,    // 第1波の群れ
        Sentinel,     // 四隅のコア
        Core,         // 中央のコア
        Count
    };

    struct CreateDesc {
        std::vector<std::string> texPaths;   // 文言の画像（Show の index はこの並び順）
        Vector2 size{ 720.0f, 76.0f };
        float   centerY = 160.0f;            // 流れる高さ（画面上部の空きを想定）
        float   screenW = 1280.0f;
        float   slideInSec = 0.5f;           // 右端から中央まで
        float   holdSec = 1.6f;              // 中央で読ませる
        float   slideOutSec = 0.5f;          // 中央から左端へ
        int     layer = 200;                 // ゲームUI(〜102)より上、ポーズ(100000)より下
    };

    static std::unique_ptr<MissionBanner> Create(const CreateDesc& d) {
        std::unique_ptr<MissionBanner> ui(new MissionBanner());
        ui->desc_ = d;
        ui->SetLayer(d.layer);

        ui->sprites_.reserve(d.texPaths.size());
        for (const auto& p : d.texPaths) {
            auto sp = std::make_unique<Sprite>();
            sp->Initialize(p);
            sp->SetSize(d.size);
            sp->SetAnchorPoint({ 0.5f, 0.5f });
            sp->SetPosition({ -d.size.x, d.centerY }); // 出すまでは画面外
            sp->Update();
            ui->sprites_.push_back(std::move(sp));
        }
        return ui;
    }

    // 指定の文言を1回流す。流れている最中に呼ばれたら、そちらへ差し替えて頭から流し直す。
    void Show(int index) {
        if (index < 0 || index >= static_cast<int>(sprites_.size())) return;
        current_ = index;
        timer_ = 0.0f;
        playing_ = true;
    }

    bool IsPlaying() const { return playing_; }

    void Update() override {
        if (!playing_) return;

        // ポーズ中は止めたいのでゲーム時間で進める（TimeScale=0 で自然に止まる）
        timer_ += TimeManager::GetInstance()->GetDeltaTime();

        const float total = desc_.slideInSec + desc_.holdSec + desc_.slideOutSec;
        if (timer_ >= total) {
            playing_ = false;
            return;
        }

        const float centerX = desc_.screenW * 0.5f;
        const float offX = desc_.screenW * 0.5f + desc_.size.x * 0.5f; // 画面外までの距離

        float x;
        if (timer_ < desc_.slideInSec) {
            const float t = Ease(timer_ / desc_.slideInSec);
            x = (centerX + offX) + (centerX - (centerX + offX)) * t;   // 右外 → 中央
        }
        else if (timer_ < desc_.slideInSec + desc_.holdSec) {
            x = centerX;                                               // 中央で読ませる
        }
        else {
            const float t = Ease((timer_ - desc_.slideInSec - desc_.holdSec) / desc_.slideOutSec);
            x = centerX + ((centerX - offX) - centerX) * t;            // 中央 → 左外
        }

        sprites_[current_]->SetPosition({ x, desc_.centerY });
        sprites_[current_]->Update();
    }

    void Draw() override {
        if (!playing_) return;
        sprites_[current_]->Draw();
    }

private:
    MissionBanner() = default;

    // 出入りを柔らかくする（等速だと機械的に見える）
    static float Ease(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    CreateDesc desc_;
    std::vector<std::unique_ptr<Sprite>> sprites_;
    int   current_ = 0;
    float timer_ = 0.0f;
    bool  playing_ = false;
};

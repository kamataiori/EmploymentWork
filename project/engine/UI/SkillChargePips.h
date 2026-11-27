#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

//======================================================
// SkillChargePips（スキルの残り使用回数を点で示すUI）
//------------------------------------------------------
// ・スキルアイコンの真上に、使用回数のぶんだけ小さなピップを横並びで置く。
// ・残っている数だけ「点灯」を描き、使い切った枠は「消灯」を描くので、
//   3回中いくつ残っているかが一目で分かる（枠自体は消えない）。
// ・点灯/消灯は色の乗算ではなくテクスチャ2枚で分ける
//   （このプロジェクトのUIはテクスチャ本来の色を活かす方針のため）。
// ・値は SetRemaining() で外から渡す。UIData には回数の口が無く、
//   スキルごとに別々の値が要るため、Scene が実体を持って毎フレーム流し込む。
// ・ヘッダオンリー：PlayerHpPipBar と同じく .cpp を増やさずに UIManager へ乗せる。
//======================================================
class SkillChargePips : public UIElement {
public:
    struct CreateDesc {
        std::string onTexPath  = "Resources/charge_pip_on.png";
        std::string offTexPath = "Resources/charge_pip_off.png";
        Vector2 center{ 0.0f, 0.0f };    // 並びの中心（スキルアイコンの真上を想定）
        Vector2 pipSize{ 16.0f, 6.0f };  // ピップ1枚のサイズ
        float   spacing = 4.0f;          // ピップ間のすき間
        int     maxCharges = 3;
        int     layer = 102;
    };

    static std::unique_ptr<SkillChargePips> Create(const CreateDesc& d) {
        std::unique_ptr<SkillChargePips> ui(new SkillChargePips());
        ui->desc_ = d;
        ui->SetLayer(d.layer);

        // 中心を基準に左右対称へ並べる
        const float total = d.maxCharges * d.pipSize.x + (d.maxCharges - 1) * d.spacing;
        const float startX = d.center.x - total * 0.5f;

        ui->on_.reserve(d.maxCharges);
        ui->off_.reserve(d.maxCharges);
        for (int i = 0; i < d.maxCharges; ++i) {
            const float px = startX + i * (d.pipSize.x + d.spacing);

            auto makePip = [&](const std::string& tex) {
                auto sp = std::make_unique<Sprite>();
                sp->Initialize(tex);
                sp->SetSize(d.pipSize);
                sp->SetAnchorPoint({ 0.0f, 0.5f }); // 左端・縦中央基準で横に並べる
                sp->SetPosition({ px, d.center.y });
                sp->Update();
                return sp;
            };
            ui->on_.push_back(makePip(d.onTexPath));
            ui->off_.push_back(makePip(d.offTexPath));
        }

        ui->remaining_ = d.maxCharges; // 初期は満タン
        return ui;
    }

    // 残り回数を反映（左から点灯）
    void SetRemaining(int remaining) {
        remaining_ = std::clamp(remaining, 0, desc_.maxCharges);
    }

    void Update() override {}

    void Draw() override {
        for (int i = 0; i < desc_.maxCharges; ++i) {
            if (i < remaining_) {
                on_[i]->Draw();
            }
            else {
                off_[i]->Draw();
            }
        }
    }

private:
    SkillChargePips() = default;

    CreateDesc desc_;
    std::vector<std::unique_ptr<Sprite>> on_;
    std::vector<std::unique_ptr<Sprite>> off_;
    int remaining_ = 0;
};

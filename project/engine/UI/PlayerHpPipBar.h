// PlayerHpPipBar.h
#pragma once
#include "UIElement.h"
#include "Sprite.h"
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#undef max

//======================================================
// PlayerHpPipBar（HP.png を横に並べて表示するプレイヤーHPバー）
//------------------------------------------------------
// ・1枚の HP.png ＝ kHpPerPip_ HP として、最大HP分のピップを横並びで作る。
// ・現在HPに応じて左から必要枚数だけ描画する（右端から欠けていく）。
// ・値は ApplyData(UIData{hp,maxHp}) もしくは SetHp() で外から渡す。
// ・テクスチャ本来の色を活かすため色は乗算しない（既定 {1,1,1,1}）。
// ・ヘッダオンリー：新規 .cpp を増やさず UIManager/UIElement にそのまま乗せる。
//======================================================
class PlayerHpPipBar : public UIElement {
public:
    struct CreateDesc {
        std::string pipTexPath = "Resources/HP.png";
        Vector2 startPos{ 48.0f, 580.0f }; // 左端基準（アンカー左・縦中央）の表示開始位置
        Vector2 pipSize{ 8.0f, 24.0f };    // ピップ1枚のサイズ
        float   spacing = 2.0f;            // ピップ間のすき間
        int     maxHp = 275;               // 最大HP
        int     hpPerPip = 5;              // ピップ1枚あたりのHP
        Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f }; // テクスチャ本来の色（乗算しない）
        int     layer = 100;
    };

    static std::unique_ptr<PlayerHpPipBar> Create(const CreateDesc& d) {
        std::unique_ptr<PlayerHpPipBar> ui(new PlayerHpPipBar());
        ui->desc_ = d;
        ui->SetLayer(d.layer);

        // 最大HPを満たすのに必要なピップ枚数（端数は切り上げ）
        ui->maxPipCount_ = (d.hpPerPip > 0)
            ? static_cast<int>(std::ceil(static_cast<float>(d.maxHp) / d.hpPerPip))
            : 0;

        ui->pips_.reserve(ui->maxPipCount_);
        for (int i = 0; i < ui->maxPipCount_; ++i) {
            auto sp = std::make_unique<Sprite>();
            sp->Initialize(d.pipTexPath);
            sp->SetSize(d.pipSize);
            sp->SetAnchorPoint({ 0.0f, 0.5f }); // 左端・縦中央基準で横に並べる
            sp->SetColor(d.color);
            const float px = d.startPos.x + i * (d.pipSize.x + d.spacing);
            sp->SetPosition({ px, d.startPos.y });
            sp->Update();
            ui->pips_.push_back(std::move(sp));
        }

        ui->visibleCount_ = ui->maxPipCount_; // 初期は満タン
        return ui;
    }

    // 現在HPを反映（左から必要枚数だけ表示する）
    void SetHp(int hp) {
        const int pip = (desc_.hpPerPip > 0) ? desc_.hpPerPip : 1;
        int count = static_cast<int>(std::ceil(static_cast<float>(std::max(0, hp)) / pip));
        visibleCount_ = std::clamp(count, 0, maxPipCount_);
    }

    // UIManager::ApplyDataToAll からの注入口（hp は data.hp に入っている）
    void ApplyData(const UIData& data) override {
        SetHp(static_cast<int>(data.hp));
    }

    void Update() override {}

    void Draw() override {
        for (int i = 0; i < visibleCount_; ++i) {
            pips_[i]->Draw();
        }
    }

private:
    PlayerHpPipBar() = default;

    CreateDesc desc_;
    std::vector<std::unique_ptr<Sprite>> pips_;
    int maxPipCount_ = 0;
    int visibleCount_ = 0;
};

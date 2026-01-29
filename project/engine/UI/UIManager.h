#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "UIElement.h"

class UIManager {
public:
    void Update();
    void Draw();

    void Add(std::unique_ptr<UIElement> ui);

    // データ注入
    void ApplyDataToAll(const UIElement::UIData& data) {
        for (auto& ui : uiList_) {
            ui->ApplyData(data);
        }
    }

    // ポーズ中か？
    bool IsPaused() const { return isPaused_; }

    // PauseScreen から呼ばれる
    void SetPaused(bool paused) { isPaused_ = paused; }

private:
    std::vector<std::unique_ptr<UIElement>> uiList_;

    bool isPaused_ = false;
};

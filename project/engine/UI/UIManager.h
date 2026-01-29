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

    // モーダルUIが有効か？
    bool IsModalActive() const;

private:
    std::vector<std::unique_ptr<UIElement>> uiList_;
};

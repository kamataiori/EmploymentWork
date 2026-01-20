#include "UIManager.h"

void UIManager::Update()
{
    for (auto& ui : uiList_) {
        if (ui->IsActive()) {
            ui->Update();
        }
    }
}

void UIManager::Draw()
{
    std::sort(uiList_.begin(), uiList_.end(),
        [](const std::unique_ptr<UIElement>& a,
            const std::unique_ptr<UIElement>& b) {
                return a->GetLayer() < b->GetLayer();
        });

    for (auto& ui : uiList_) {
        if (ui->IsActive()) {
            ui->Draw();
        }
    }
}

void UIManager::Add(std::unique_ptr<UIElement> ui)
{
    uiList_.push_back(std::move(ui));
}

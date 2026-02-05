#include "UIManager.h"

void UIManager::Update()
{
    const bool modal = IsModalActive();

    for (auto& ui : uiList_) {
        if (!ui->IsActive()) continue;

        // モーダルが居る時は「モーダルUIだけUpdate」
        if (modal) {
            if (ui->IsModal()) {
                ui->Update();
            }
        }
        else {
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

bool UIManager::IsModalActive() const
{
    for (const auto& ui : uiList_) {
        if (ui->IsActive() && ui->IsModal()) {
            return true;
        }
    }
    return false;
}

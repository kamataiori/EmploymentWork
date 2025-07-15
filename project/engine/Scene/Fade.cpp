#include "Fade.h"

void Fade::Initialize() {
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("Resources/Black.png");
    sprite_->SetSize({ 1280, 720 });
    sprite_->SetPosition({ 0, 0 });
}

void Fade::Start(Status status, float duration) {
    status_ = status;
    duration_ = duration;
    counter_ = 0.0f;
}

void Fade::Stop() {
    status_ = Status::Finish;
}

void Fade::Update() {
    if (status_ == Status::None) return;

    counter_ += 1.0f / 60.0f;
    counter_ = (counter_ < duration_) ? counter_ : duration_;

    if (status_ == Status::FadeIn) {
        alpha_ = 1.0f - std::clamp(counter_ / duration_, 0.0f, 1.0f);
    }
    else if (status_ == Status::FadeOut) {
        alpha_ = std::clamp(counter_ / duration_, 0.0f, 1.0f);
    }

    sprite_->SetColor({ 0, 0, 0, alpha_ });

    if (counter_ >= duration_) {
        Stop(); // 自動で停止
    }

    sprite_->Update();
}

void Fade::Draw() {
    if (status_ == Status::None) return;
    sprite_->Draw();
}

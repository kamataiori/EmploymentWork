#pragma once
#include "Sprite.h"
#include <memory>
#include <algorithm>

class Fade {
public:
    enum class Status {
        None,     // フェードなし
        FadeIn,   // フェードイン中
        FadeOut,   // フェードアウト中
        Finish,
    };

    void Initialize();
    void Start(Status status, float duration);
    void Stop();

    void Update();
    void Draw();

    bool IsActive() const { return status_ != Status::None; }
    bool IsFinish() const { return status_ != Status::Finish; }
    bool IsFadeOutComplete() const { return status_ == Status::None && alpha_ >= 1.0f; }

private:
    std::unique_ptr<Sprite> sprite_ = nullptr;

    Status status_ = Status::None;
    // フェードの持続時間
    float duration_ = 0.0f;
    // 経過時間カウンター
    float counter_ = 0.0f;
    float alpha_ = 1.0f;
};

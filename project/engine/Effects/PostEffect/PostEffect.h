#pragma once
#include <memory>
#include "OffscreenRendering.h"

class PostEffect {
public:
    void Initialize(PostEffectType type);
    void Draw();
    void SetType(PostEffectType type);
    PostEffectType GetType() const;

    OffscreenRendering* GetOffscreen() const { return offscreen_.get(); }

    uint32_t GetSrvIndex() const {
        return offscreen_->GetSrvIndex();
    }

private:
    std::unique_ptr<OffscreenRendering> offscreen_ = std::make_unique<OffscreenRendering>();
};

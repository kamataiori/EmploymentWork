// PostEffectManager.h
#pragma once
#include <memory>
#include "PostEffect.h"

class PostEffectManager {
public:
    static PostEffectManager* GetInstance();

    void Initialize(PostEffectType type);
    void Draw();
    void Finalize();

    void SetType(PostEffectType type);
    PostEffectType GetType() const;

    uint32_t GetSrvIndex() const;
    OffscreenRendering* GetOffscreen();

private:
    PostEffectManager() = default;
    ~PostEffectManager() = default;
    PostEffectManager(const PostEffectManager&) = delete;
    PostEffectManager& operator=(const PostEffectManager&) = delete;

private:
    std::unique_ptr<PostEffect> postEffect_ = nullptr;
};

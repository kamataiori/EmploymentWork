#pragma once

class UIElement {
public:
    virtual ~UIElement() = default;

    virtual void Update() {}
    virtual void Draw() = 0;

    struct UIData {
        float hp = 0.0f;
        float maxHp = 1.0f;
    };

    // ここが重要：派生へデータを注入する共通口
    virtual void ApplyData(const UIData& data) { (void)data; }

    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    void SetLayer(int layer) { layer_ = layer; }
    int GetLayer() const { return layer_; }

protected:
    bool isActive_ = true;
    int layer_ = 0;
};

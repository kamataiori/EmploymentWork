#pragma once
#include "DrawLine.h"

class Debug
{
public:

    /// <summary>
    /// 初期化処理
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// ライン描画の追加
    /// </summary>
    void AddLine(const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
        Color startColor, Color endColor);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

private:

    // DrawLine のインスタンス
    std::unique_ptr<DrawLine> drawLine_;
};

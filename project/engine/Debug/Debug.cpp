#include "Debug.h"

// 初期化
void Debug::Initialize()
{
    drawLine_ = std::make_unique<DrawLine>();
    drawLine_->Initialize();
}

// 終了
void Debug::Finalize()
{
    drawLine_.reset();
}

// ライン描画の追加
void Debug::AddLine(const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
    Color startColor, Color endColor)
{
    if (drawLine_) {
        drawLine_->AddLine(startPoint, endPoint, startColor, endColor);
    }
}

void Debug::Update()
{
    drawLine_->Update();
}

// 描画
void Debug::Draw()
{
    if (drawLine_) {
        drawLine_->Draw();
    }
}

#pragma once
#include "Flow/GameFlowState.h"
#include <Sprite.h>
#include <array>
#include <memory>

//======================================================
// IntroState
//------------------------------------------------------
// バトル開始前の演出。5→0 のカウントダウンのあと "START!" を出して戦闘へ。
// この間プレイヤーの入力はロックし、敵はAIを止めて見た目だけ更新する。
//
// 「ステージとプレイヤーを映すカメラ演出 → 敵の登場演出」を入れるときは、
// この手前に State を足して数珠つなぎにする（この State は触らずに済む）。
//======================================================
class IntroState : public GameFlowState
{
public:
    // スプライトの読み込みはコンストラクタで行う（シーン初期化時に構築されるため、
    // 演出の1フレーム目でテクスチャ読み込みが走って引っ掛かるのを避ける）
    IntroState();

    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    void Exit(GameFlowContext& ctx) override;
    void UIDraw(GameFlowContext& ctx) override;

    const char* GetName() const override { return "Intro"; }

private:
    static constexpr int   kCountMax        = 5;      // 5 から 0 まで表示する
    static constexpr float kCountPerSec     = 1.0f;   // 1数字あたりの秒数
    static constexpr float kStartDisplaySec = 1.2f;   // "START!" を出しておく秒数

    std::array<std::unique_ptr<Sprite>, kCountMax + 1> countSprites_;
    std::unique_ptr<Sprite> startSprite_;

    int   countdownNum_   = kCountMax;
    float countdownTimer_ = 0.0f;

    // カウントダウンを終えて "START!" を表示している段階か
    bool  showingStartCue_   = false;
    float startDisplayTimer_ = 0.0f;
};

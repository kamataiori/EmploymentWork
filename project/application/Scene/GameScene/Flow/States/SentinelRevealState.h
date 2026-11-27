#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// SentinelRevealState（前哨の敵 出現カットシーン）
//------------------------------------------------------
// 第1波（手前の群れ）を殲滅した直後の局面。カメラを奥の四角エリアへ
// 切り替え、四隅の Sentinel が地中から迫り上がる様子を見せる。
// 見せ終わったらプレイヤーの followCamera へ戻し、AdvanceState（奥へ歩く）
// へ引き継ぐ。
//
// カメラは FollowCamera のシネマティック上書き（SetCinematicBlend）を使い、
// weight を 0→1→0 と補間して追従⇔演出の出入りを滑らかにする。
// この間はプレイヤー入力をロックする（演出をじっくり見せる）。
//======================================================
class SentinelRevealState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    void Exit(GameFlowContext& ctx) override;
    const char* GetName() const override { return "SentinelReveal"; }

private:
    // カットシーン全体の進行時間（unscaled 実時間）
    float timer_ = 0.0f;

    // カメラ到着後に一度だけ StartSpawn するためのフラグ
    bool spawnStarted_ = false;

    // ---- 演出の尺（秒）----
    // Blend を長くするほどカメラの寄り／戻りがゆっくりになる
    static constexpr float kBlendIn  = 1.5f;  // 追従→演出カメラへ寄る
    static constexpr float kHold     = 1.6f;  // 出現を見せる（迫り上がり0.8秒＋余韻）
    static constexpr float kBlendOut = 1.5f;  // 演出カメラ→追従へ戻る
    static constexpr float kTotal    = kBlendIn + kHold + kBlendOut;

    // ---- 演出カメラの構図（奥アリーナ中心を基準にした相対位置）----
    static constexpr float kCamBackOffset = 60.0f; // 中心より手前(-Z)側へ下がる距離
    static constexpr float kCamHeight     = 28.0f; // 中心からのカメラ高さ
    static constexpr float kLookHeight    = 6.0f;  // 注視点の高さ（迫り上がる敵を捉える）
};

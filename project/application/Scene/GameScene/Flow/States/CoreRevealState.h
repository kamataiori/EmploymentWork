#pragma once
#include "Flow/States/BattlePhaseState.h"

//======================================================
// CoreRevealState（中央コア 出現カットシーン）
//------------------------------------------------------
// 四隅の Sentinel を全滅させた直後の局面。カメラを中央コアへ寄せ、
// コアが地中から迫り上がる様子を見せてから、プレイヤーの followCamera
// へ戻し、CoreBattleState（コア戦）へ引き継ぐ。
//
// 作りは SentinelRevealState と同じ：
//   FollowCamera のシネマティック上書き（SetCinematicBlend）で weight を
//   0→1→0 と補間し、カメラが着いた時点（blend-in 完了）で StartSpawn する。
// この間はプレイヤー入力をロックする。
//======================================================
class CoreRevealState : public BattlePhaseState
{
public:
    void Enter(GameFlowContext& ctx) override;
    void Update(GameFlowContext& ctx) override;
    void Exit(GameFlowContext& ctx) override;
    const char* GetName() const override { return "CoreReveal"; }

private:
    // カットシーン全体の進行時間（unscaled 実時間）
    float timer_ = 0.0f;

    // カメラ到着後に一度だけ StartSpawn するためのフラグ
    bool spawnStarted_ = false;

    // ---- 演出の尺（秒）----
    static constexpr float kBlendIn  = 1.5f;  // 追従→演出カメラへ寄る
    static constexpr float kHold     = 1.6f;  // 出現を見せる（迫り上がり0.8秒＋余韻）
    static constexpr float kBlendOut = 1.5f;  // 演出カメラ→追従へ戻る
    static constexpr float kTotal    = kBlendIn + kHold + kBlendOut;

    // ---- 演出カメラの構図（奥アリーナ中心＝コアの位置を基準にした相対位置）----
    // 四隅（SentinelReveal）は一辺64の正方形を収める引きの画だが、こちらは
    // コア1体（スケール4・高さ約14）だけを見せるので寄り気味にする。
    static constexpr float kCamBackOffset = 32.0f; // 中心より手前(-Z)側へ下がる距離
    static constexpr float kCamHeight     = 14.0f; // 中心からのカメラ高さ
    static constexpr float kLookHeight    = 7.0f;  // 注視点の高さ（コアの中ほど）
};

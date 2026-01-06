#pragma once
#include <memory>
#include <functional>
#include "BehaviorTree.h"

// 前方宣言
class Enemy;
struct Transform;

//======================================================
// EnemyAIController
//------------------------------------------------------
// ・Enemyの「思考部分」だけを担当するクラス
// ・Enemy本体は「移動・アニメ・当たり判定」
// ・AIControllerは「何をするか（判断）」
//
// この分離により
// ・AIの差し替えが簡単
// ・ステート（Phase）× BT の併用が可能
// ・テストやデバッグがしやすい
//
// Enemy::Update()
//   → aiController_->Update(dt);
// という形で使う
//======================================================
class EnemyAIController {
public:
    EnemyAIController() = default;

    // Enemyと紐付け、BehaviorTreeを構築
    void Initialize(Enemy* owner);

    // 毎フレーム更新
    void Update(float dt);

    //==================================================
    // 外部から設定できるインターフェース
    //==================================================

     // ターゲット取得方法を外から注入
    // Scene側で「今のPlayer」を返す関数を渡せる
    void SetTargetGetter(std::function<const Transform* ()> getter) { targetGetter_ = std::move(getter); }

    // 追跡開始距離（この距離より遠ければ追跡）
    void SetChaseStartDistance(float d) { chaseStartDist_ = d; }

    // 停止距離（この距離より近ければ追跡終了）
    void SetStopDistance(float d) { stopDist_ = d; }

    // 追跡速度
    void SetChaseSpeed(float s) { chaseSpeed_ = s; }

    // 向き補間率（旋回の滑らかさ）
    void SetTurnLerp(float t) { turnLerp_ = t; }

private:
    //========================
    // BehaviorTree 構築
    //========================
    void BuildTree();

    //========================
    // ノード実処理（Condition/Action から呼ぶ）
    //========================
    
    // ターゲット探索（ctx.target にセット）
    bool FindTarget(BT::Context& ctx);

    // ターゲットが「遠いか？」
    bool IsTargetFar(BT::Context& ctx) const;

    // 追跡処理（移動＋回転＋アニメ）
    BT::Status Chase(BT::Context& ctx);

private:
    Enemy* owner_ = nullptr;   // 思考対象のEnemy
    BT::Tree tree_;            // このEnemy専用のBT

    // ターゲット取得用コールバック
    std::function<const Transform* ()> targetGetter_{};

    //==================================================
    // AIパラメータ（ImGui調整向け）
    //==================================================
    
    // 追跡パラメータ
    float chaseStartDist_ = 12.0f;
    float stopDist_ = 3.0f;
    float chaseSpeed_ = 0.22f;
    float turnLerp_ = 0.18f;
};

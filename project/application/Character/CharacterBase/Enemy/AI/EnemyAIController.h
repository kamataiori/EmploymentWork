#pragma once
#include <memory>
#include <functional>

// Node方式 BT基盤
#include "application/AI/BehaviorTree/Core/BlackBoard.h"
#include "application/AI/BehaviorTree/Core/INode.h"

// Forward
class Enemy;
struct Transform;

//======================================================
// EnemyAIController（Node方式）
// ・Enemyは「aiController_->Update()」を呼ぶだけ
// ・このクラスは「BT構築＆実行」担当
//======================================================
class EnemyAIController {
public:
    EnemyAIController() = default;
    ~EnemyAIController() = default;

    // Enemyと紐付け、ツリー構築
    void Initialize(Enemy* owner);

    // 毎フレーム実行
    void Update(float dt);

    // Scene側から「今のPlayerのTransform」を返す関数を注入
    void SetTargetGetter(std::function<const Transform* ()> getter) { targetGetter_ = std::move(getter); }

    // 追跡開始距離（この距離より遠ければ追跡）
    void SetChaseStartDistance(float d) {
        chaseStartDist_ = d;
        FixHysteresis();
    }

    // 停止距離（この距離より近ければ追跡終了）
    void SetStopDistance(float d) {
        stopDist_ = d;
        FixHysteresis();
    }

    // 追跡速度
    void SetChaseSpeed(float s) { chaseSpeed_ = s; }

    // 向き補間率（旋回の滑らかさ）
    void SetTurnLerp(float t) { turnLerp_ = t; }

    void SetAttackDistance(float d) { attackDist_ = d; }


private:
    void BuildTree();

    // ヒステリシス修正
    // ガタつき防止
    void FixHysteresis();

private:
    Enemy* owner_ = nullptr;

    // Blackboard / Root
    std::unique_ptr<BlackBoard> blackboard_;
    std::unique_ptr<INode> root_;

    // ターゲット取得用
    std::function<const Transform* ()> targetGetter_{};

    // パラメータ
    float chaseStartDist_ = 12.0f;
    float stopDist_ = 3.0f;
    float chaseSpeed_ = 15.0f;
    float turnLerp_ = 0.18f;
    float attackDist_ = 3.0f; // パンチ開始距離（追跡停止距離とは分ける）

};

#pragma once
#include "EnemyActionState.h"
#include "MathFunctions.h"

//======================================================
// SummonMinionState
//------------------------------------------------------
// ボスが雑魚敵を召喚する攻撃ステート
//
// フェーズ:
//   Summon  → 雑魚敵を3〜5体一斉に出現させる
//   Wait    → 召喚後に少し硬直（演出用）
//   完了    → BTに制御を返す
//
// 雑魚敵はボスの周囲にランダム配置で出現し、
// 自律的にプレイヤーに向かって移動する
//======================================================
class SummonMinionState : public EnemyActionState
{
public:
	SummonMinionState() = default;

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "SummonMinion"; }

private:
	enum class Phase {
		Summon,   // 召喚実行
		Wait,     // 硬直
	};

	Phase phase_ = Phase::Summon;
	float timer_ = 0.0f;

	// 召喚数（3〜5体のランダム）
	int summonCount_ = 4;

	// 召喚後の硬直時間
	float waitDuration_ = 1.0f;

	// 出現半径（ボスの周囲にどれくらい離して出すか）
	float spawnRadius_ = 5.0f;
};
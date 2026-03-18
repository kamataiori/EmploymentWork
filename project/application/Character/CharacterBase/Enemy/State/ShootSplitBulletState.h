#pragma once
#include "EnemyActionState.h"

//======================================================
// ShootSplitBulletState
//------------------------------------------------------
// 分裂弾をプレイヤーに向けて発射するステート
//
// フェーズ:
//   WindUp  → 発射前の溜め（プレイヤーを向く）
//   Shoot   → 分裂弾を生成・発射
//   Recover → 発射後の硬直
//   完了    → BT に制御を返す
//======================================================
class ShootSplitBulletState : public EnemyActionState
{
public:
	// angryMode: true のとき SpawnSplitBurstAngry（8発）を使う
	explicit ShootSplitBulletState(bool angryMode = false);

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "ShootSplitBullet"; }

private:
	enum class Phase {
		WindUp,   // 溜め（プレイヤーを向く）
		Shoot,    // 発射
		Recover,  // 硬直
	};

	bool  angryMode_ = false; // true のとき8発モード
	Phase phase_ = Phase::WindUp;
	float timer_ = 0.0f;

	// 溜め時間（秒）
	float windUpTime_ = 0.6f;

	// 発射後の硬直時間（秒）
	float recoverTime_ = 0.8f;
};
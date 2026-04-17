#pragma once
#include "EnemyActionState.h"

//======================================================
// ThrowBigBulletState
//------------------------------------------------------
// 大弾をプレイヤーに向けて投げるステート
//
// フェーズ:
//   WindUp  → 予備動作（プレイヤーを向く・警告）1.5秒
//   Throw   → 大弾生成・発射
//   Recover → 硬直（プレイヤーの反撃チャンス）1.0秒
//
// フェーズ2以降で使用想定
//======================================================
class ThrowBigBulletState : public EnemyActionState
{
public:
	ThrowBigBulletState() = default;

	void Enter(Enemy* enemy) override;
	bool Update(Enemy* enemy, float dt) override;
	void Exit(Enemy* enemy) override;

	const char* GetName() const override { return "ThrowBigBullet"; }

private:
	enum class Phase {
		WindUp,   // 予備動作
		Throw,    // 投擲
		Recover,  // 硬直
	};

	Phase phase_ = Phase::WindUp;
	float timer_ = 0.0f;
	float windUpTime_ = 1.5f;  // 溜め時間（プレイヤーが回避を準備できる）
	float recoverTime_ = 1.0f;  // 硬直時間（反撃チャンス）
};
#include "SummonMinionState.h"
#include "Enemy/Enemy.h"
#include <random>
#include <cmath>
#include <numbers>

void SummonMinionState::Enter(Enemy* enemy)
{
	phase_ = Phase::Summon;
	timer_ = 0.0f;

	// 召喚数を3〜5のランダムで決定
	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(3, 5);
	summonCount_ = dist(rng);
}

bool SummonMinionState::Update(Enemy* enemy, float dt)
{
	switch (phase_)
	{
	case Phase::Summon:
	{
		// ボスの周囲に均等配置で雑魚敵を召喚
		Vector3 bossPos = enemy->GetTransform().translate;

		// 雑魚敵を等間隔の角度で配置
		float angleStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(summonCount_);

		for (int i = 0; i < summonCount_; ++i) {
			float angle = angleStep * static_cast<float>(i);

			Vector3 spawnPos = bossPos;
			spawnPos.x += std::cos(angle) * spawnRadius_;
			spawnPos.z += std::sin(angle) * spawnRadius_;
			spawnPos.y = bossPos.y; // 地面に合わせる

			enemy->SpawnMinion(spawnPos);
		}

		// 召喚アニメーション（あれば）
		if (!enemy->IsHitReact()) {
			enemy->SetAnimationIfChanged(enemy->GetAnimSet().Wave);
		}

		phase_ = Phase::Wait;
		timer_ = 0.0f;
		break;
	}

	case Phase::Wait:
	{
		// 硬直時間
		timer_ += dt;
		if (timer_ >= waitDuration_) {
			return false; // ★ 完了
		}
		break;
	}
	}

	return true; // まだ実行中
}

void SummonMinionState::Exit(Enemy* enemy)
{
	if (!enemy->IsHitReact()) {
		enemy->SetAnimationIfChanged(enemy->GetAnimSet().Idle);
	}
}
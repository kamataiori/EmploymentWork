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
		// ボスの半径 spawnRadius_ 以内にランダムで雑魚敵を召喚
		Vector3 bossPos = enemy->GetTransform().translate;

		static std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
		std::uniform_real_distribution<float> unitDist(0.0f, 1.0f);

		for (int i = 0; i < summonCount_; ++i) {
			float angle = angleDist(rng);
			// sqrt を掛けて円の内部に一様分布させる
			float r = spawnRadius_ * std::sqrt(unitDist(rng));

			Vector3 spawnPos = bossPos;
			spawnPos.x += std::cos(angle) * r;
			spawnPos.z += std::sin(angle) * r;
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
			return false; // 完了
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
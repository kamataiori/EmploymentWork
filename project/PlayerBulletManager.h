// ===============================
// 弾の生成/更新/破棄を一括管理。Weapon側やPlayer側から利用
// ===============================
#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "PlayerIBullet.h"

class PlayerBulletManager {
public:
	void Initialize(BaseScene* scene) { scene_ = scene; }


	// TはPlayerIBullet派生
	template<class T, class... Args>
	T* Spawn(Args&&... args) {
		auto b = std::make_unique<T>(std::forward<Args>(args)...);
		b->Initialize(scene_);
		T* raw = b.get();
		bullets_.emplace_back(std::move(b));
		return raw;
	}


	void Update() {
		for (auto& b : bullets_) if (b) b->Update();
		// 死亡した弾を除去
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(),
				[](const std::unique_ptr<PlayerIBullet>& b) { return !b || !b->IsAlive(); }),
			bullets_.end());
	}


	void Draw() {
		for (auto& b : bullets_) if (b) b->Draw();
	}


	void Clear() { bullets_.clear(); }


private:
	BaseScene* scene_ = nullptr;
	std::vector<std::unique_ptr<PlayerIBullet>> bullets_;
};

// ===============================
// File: 使用例（Weapon からの発射）
// PlayerWeaponOBB 等のUpdateで manager_.Update()/Draw() を呼ぶだけでOK
// 発射時:
// auto* b = manager_.Spawn<AssaultBullet>();
// b->SetOwner(owner_);
// Vector3 muzzlePos = playerTransform_->translate; // 実装環境に合わせて
// Vector3 shootDir = {0,0,1}; // forward に置換
// b->Shoot(muzzlePos, shootDir, 1.2f, 2.0f);
// ===============================
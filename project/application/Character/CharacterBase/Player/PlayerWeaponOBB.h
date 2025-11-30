#pragma once
#include "PlayerIWeapon.h"
#include "CollisionTypeIdDef.h"
#include "PlayerBullet.h"

#include <memory>
#include <vector>
#include <unordered_set>

class PlayerWeaponOBB : public PlayerIWeapon{
public:

	PlayerWeaponOBB() {}

	void Initialize() override;

	void Update() override;

	void Draw() override;
   
	void NormalAttack() override;

	void Skill() override;
	void Ultimate() override;

	// （必要ならシーン登録用に公開）
	MultiCollider* GetMultiCollider() const { return mc_.get(); }

	//// カメラを切り替えたときに使用する
	//void SetCameraForProjectiles(Camera* cam) {
	//	for (auto& b : bullets_) b->SetCamera(cam);
	//}

	//// 弾リスト（シーンでコリジョン登録する）
	//const std::vector<std::unique_ptr<PlayerBullet>>& GetBullets() const { return bullets_; }

	//// 敵の Transform（敵方向に撃つため）
	//void SetEnemyTransform(Transform* t) { enemyTransform_ = t; }

private:

	// ===== 正面OBB（剣の当たり判定） =====
	std::unique_ptr<MultiCollider> mc_{};

	// 出現中タイマー（>0の間だけ当たり有効）
	float activeDuration_ = 0.5f;      // 出現時間（秒）
	float activeTime_ = 0.0f;

	// サイズ（ハーフエクステント）
	Vector3 obbHalf_ = { 1.08f, 1.58f, 2.45f };

	// プレイヤー足元原点からのオフセット
	float frontDist_ = 0.85f;  // 前方距離
	float height_ = 1.0f;   // 高さ

	// 入力エッジ検出用
	bool IsnormalAttack_ = false;
	bool IsSkill_ = false;


	// デバッグ用フラグ
	bool showDebug_ = true;

	// ---- 弾管理 ----
	//std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	//Transform* enemyTransform_ = nullptr; // 敵の位置参照
};

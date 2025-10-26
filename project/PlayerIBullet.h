// ===============================
// Player用の弾インターフェース
// ===============================
#pragma once
#include "Transform.h"

class BaseScene;
class PlayerBase;

class PlayerIBullet {
public:
	virtual ~PlayerIBullet() = default;


	// 生成直後に必ず呼ぶ。派生でObject3dやコライダの型を確定する
	virtual void Initialize(BaseScene* scene) = 0;


	// 更新
	virtual void Update() = 0;


	// デバッグ/モデル描画
	virtual void Draw() = 0;


	// 発射パラメータをまとめてセット
	// pos: ワールド発射位置 / dir: 正規化方向 / speed: 速さ(フレーム単位) / lifeSec: 寿命秒
	virtual void Shoot(const Vector3& pos, const Vector3& dir, float speed, float lifeSec) = 0;


	// 生存管理
	virtual bool IsAlive() const = 0;


	// 所有者（プレイヤ）設定（ダメージ計算やエフェクト参照に使用）
	virtual void SetOwner(PlayerBase* owner) = 0;
};
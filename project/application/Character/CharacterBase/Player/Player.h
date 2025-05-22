#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "PlayerBullet.h"
#include <list>

class Player : public CharacterBase, public SphereCollider
{
public:
	Player(BaseScene* baseScene_) : CharacterBase(baseScene_), SphereCollider(sphere) {}

	// 初期化処理
	void Initialize() override;
	// 毎フレームの更新処理
	void Update() override;
	// 3D描画処理
	void Draw() override;
	// 2D（スプライト）描画処理
	void Draw2D();
	// デバッグ表示（ImGui）
	void Debug() override;
	// 弾の描画
	void DrawBullets();

private:
	// モデル初期化
	void InitializeModel();
	// Transform設定（初期位置・スケール・回転）
	void InitializeTransform();
	// コライダー設定
	void InitializeCollider();
	// UIスプライト初期化
	void InitializeUI();
	// UIスプライト更新
	void UpdateUI();
	// 入力処理に基づく移動
	void HandleMovement();
	// 弾発射処理
	void HandleShooting();
	// 弾の生存更新と削除処理
	void UpdateBullets();
	// Transformの各種値をObject3dに反映
	void UpdateTransform();

	// 線形補間
	inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
		return a + (b - a) * t;
	}

	// ベクトルを回転行列で変換
	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

	// ベクトルを行列で変換
	Vector3 Multiply(const Vector3& v, const Matrix4x4& m);

	// 行列同士の乗算
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	// 回転行列作成
	Matrix4x4 MakeRotateMatrix(const Vector3& rotate);

private:
	// 移動速度ベクトル
	Vector3 velocity_{};

	// レティクルスプライト
	std::unique_ptr<Sprite> reticleSprite_;

	std::unique_ptr<Object3d> test_;
	Transform testTransform_;

	// プレイヤー弾リスト
	std::list<std::unique_ptr<PlayerBullet>> bullets_;

	// 弾ストック数
	int bulletStock_ = 10;

	// 連射防止フラグ
	bool canShoot_ = true;
};

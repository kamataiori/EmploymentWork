#pragma once
#include "ObjectBase.h"
#include <string>

class Player;

class Sword : public ObjectBase
{
public:
	Sword(BaseScene* scene) : ObjectBase(scene) {}

	void Initialize() override;
	void Update() override;

	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;
	void ParticleDraw() override;
	void AnimationDraw() override;
	void OnCollision() override;

	void AttachTo(Object3d* ownerObj, const std::string& jointName);

	void SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s);

	void SetHitEnabled(bool enabled) { hitEnabled_ = enabled; }
	// 攻撃中で当たり判定が有効か（Scene のコライダー登録判定に使う）
	bool IsHitEnabled() const { return hitEnabled_; }

	MultiCollider* GetMultiCollider() const { return multiCollider_.get(); }

	/// <summary>
	/// 剣の現在のワールド座標を返す（親ボーン追従中・切り離して移動中のどちらも反映する）。
	/// オーラ等のパーティクルを剣に追従させるのに使う。
	/// </summary>
	Vector3 GetWorldPosition() const {
		const Matrix4x4& w = object3d_->GetWorldMatrix();
		return { w.m[3][0], w.m[3][1], w.m[3][2] };
	}

	// プレイヤーのTransformを参照して向き(yaw)を取る
	void SetPlayerTransform(const Transform* t) { playerTransform_ = t; }

	//======================================================
	// 武器としての汎用操作（スキルの振り付けはスキル側が行う）
	//   スキルクラスはこれらを組み合わせて剣を動かす。
	//======================================================

	/// <summary>
	/// 親ボーン追従を切り、ワールド空間で自由に動かせる状態にする。
	/// </summary>
	void Detach() { object3d_->SetParent(nullptr); }

	/// <summary>
	/// 手（親ボーン）に戻し、当たり判定・サイズも既定へリセットする。
	/// </summary>
	void Reattach();

	/// <summary>
	/// 切り離し中にワールド空間の位置・姿勢を直接指定する。
	/// </summary>
	void SetWorldTransform(const Vector3& translate, const Vector3& rotate);

	/// <summary>
	/// 当たり判定OBBの半サイズを変更する（攻撃の種類で当たり範囲を変えたいとき）。
	/// </summary>
	void SetHitHalfSize(const Vector3& halfSize) { hitHalfSize_ = halfSize; }

private:
	Object3d* ownerObj_ = nullptr;
	std::string ownerJoint_;

	const Transform* playerTransform_ = nullptr;

	bool hitEnabled_ = false;
	bool isHit_ = false;

	// OBB調整値
	Vector3 hitOffset_ = { 0.0f, 0.0f, 8.5f };
	// 通常攻撃時の当たり判定の半サイズ（既定値）。スキル等で一時的に変更され得る。
	const Vector3 kDefaultHitHalfSize_ = { 0.7f, 0.8f, 2.2f };
	Vector3 hitHalfSize_ = kDefaultHitHalfSize_;

	Vector3 defaultOffsetT_ = { -0.44f, -0.73f, -0.48f };
	Vector3 defaultOffsetR_ = { -0.56f, 4.95f, 0.0f };
	Vector3 defaultOffsetS_ = { 1.0f, 1.0f, 1.0f };
};
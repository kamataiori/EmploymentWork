#include "Sword.h"
#include <CollisionTypeIdDef.h>
#include <cmath>
#include <numbers>
#include <algorithm>

// Yaw(=Y回転)から OBB の3軸を作る
static void BuildYawAxes(float yaw, Vector3 outAxes[3])
{
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);

	// 右(X), 上(Y), 前(Z)
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f, 0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

// ローカルオフセットを yaw 回転してワールドへ
static Vector3 RotateOffsetByYaw(const Vector3& localOffset, float yaw)
{
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);

	return {
		localOffset.x * c + localOffset.z * s,
		localOffset.y,
	   -localOffset.x * s + localOffset.z * c
	};
}

void Sword::Initialize()
{
	object3d_->Initialize();

	ModelManager::GetInstance()->LoadModel("sword.obj");
	object3d_->SetModel("sword.obj");

	transform.translate = { 0.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	// ---- OBBコライダー初期化 ----
	OBB obb{};
	obb.center = { 0.0f, 0.0f, 0.0f };
	obb.orientations[0] = { 1.0f, 0.0f, 0.0f };
	obb.orientations[1] = { 0.0f, 1.0f, 0.0f };
	obb.orientations[2] = { 0.0f, 0.0f, 1.0f };
	obb.size = { 0.0f, 0.0f, 0.0f }; // 最初は無効

	Shape shape{};
	shape.kind = ShapeKind::OBB;
	shape.obb = obb;

	*multiCollider_ = MultiCollider(shape);

	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerWeapon));
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });
}

void Sword::AttachTo(Object3d* ownerObj, const std::string& jointName)
{
	ownerObj_ = ownerObj;
	ownerJoint_ = jointName;

	object3d_->SetParentJoint(ownerObj_, ownerJoint_);

	transform.translate = defaultOffsetT_;
	transform.rotate = defaultOffsetR_;
	transform.scale = defaultOffsetS_;
}

void Sword::SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s)
{
	transform.translate = t;
	transform.rotate = r;
	transform.scale = s;
}

void Sword::Update()
{
	// スキルの弧モード中は、親ボーンではなくワールド空間の弧上に剣を配置する
	if (spinArcActive_) {
		UpdateSpinArcTransform();
	}

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// ---- OBB更新 ----
	OBB& obb = multiCollider_->MutableOBB(0);

	const Matrix4x4& world = object3d_->GetWorldMatrix();

	// 行から軸を取り出す
	Vector3 axisX = Normalize({ world.m[0][0], world.m[0][1], world.m[0][2] });
	Vector3 axisY = Normalize({ world.m[1][0], world.m[1][1], world.m[1][2] });
	Vector3 axisZ = Normalize({ world.m[2][0], world.m[2][1], world.m[2][2] });

	Vector3 worldPos = world.Translation();

	obb.center = worldPos;

	obb.orientations[0] = axisX;
	obb.orientations[1] = axisY;
	obb.orientations[2] = axisZ;

	// 当たり判定のサイズ：
	//   スキル(弧モード)中 … 常時ON＋大きめの kSpinArcHitHalfSize_
	//   通常攻撃中         … hitEnabled_ の区間だけ通常サイズ hitHalfSize_
	//   それ以外           … 0（無効）
	if (spinArcActive_) {
		obb.size = kSpinArcHitHalfSize_;
	}
	else {
		obb.size = hitEnabled_ ? hitHalfSize_ : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	isHit_ = false;
}

void Sword::BeginSpinArc()
{
	spinArcActive_ = true;
	spinArcProgress_ = 0.0f;

	// 発動した瞬間のプレイヤーの向きを固定する。
	// 弧の「方向」はこの値で決め打ちにし、スキル中に向きを変えても弧は振り回されない。
	// （位置は毎フレーム追従するので、移動しても手元へ戻る）
	spinArcYaw_ = playerTransform_ ? playerTransform_->rotate.y : 0.0f;

	// 親ボーン追従を切り、ワールド空間で自由に動かせるようにする
	object3d_->SetParent(nullptr);
}

void Sword::EndSpinArc()
{
	spinArcActive_ = false;

	// 手に戻す：親ボーン追従を復帰し、ローカルオフセットを既定値へ戻す
	object3d_->SetParentJoint(ownerObj_, ownerJoint_);
	transform.translate = defaultOffsetT_;
	transform.rotate = defaultOffsetR_;
	transform.scale = defaultOffsetS_;
}

void Sword::UpdateSpinArcTransform()
{
	// 位置は毎フレーム参照（プレイヤーが移動しても弧が追従して手元へ戻る）。
	Vector3 playerPos = playerTransform_ ? playerTransform_->translate : Vector3{ 0.0f, 0.0f, 0.0f };

	// 向きは発動時に固定した値を使う（スキル中に向きを変えても弧は振り回されない）。
	const float playerYaw = spinArcYaw_;

	const float t = std::clamp(spinArcProgress_, 0.0f, 1.0f);

	// プレイヤー正面(forward = (sinYaw, 0, cosYaw))を基準に、左→右へ角度を振る
	const float sweepAngle = kArcStartAngle_ + (kArcEndAngle_ - kArcStartAngle_) * t;
	const float worldAngle = playerYaw + sweepAngle;
	const Vector3 dir = { std::sin(worldAngle), 0.0f, std::cos(worldAngle) };

	// 半径・高さは sin(πt) の山なり：t=0と1で手元(=0)に戻り、t=0.5で最大になる
	const float arcEnvelope = std::sin(std::numbers::pi_v<float> *t); // 0 → 1 → 0
	const float radius = kArcMaxRadius_ * arcEnvelope;
	const float height = kArcBaseHeight_ + kArcLiftHeight_ * arcEnvelope;

	// プレイヤーを中心に弧を描くワールド座標
	transform.translate = {
		playerPos.x + dir.x * radius,
		playerPos.y + height,
		playerPos.z + dir.z * radius
	};

	// 自転（ぐるぐる）：横向き(寝た状態)を保ったまま、垂直軸まわりに回す。
	//   プロペラ／時計の針のように水平回転する見た目になる。
	//   Y に弧の向き(worldAngle)＋自転角を加える。
	const float spinAngle = kArcSpinTurns_ * 2.0f * std::numbers::pi_v<float> *t;
	transform.rotate = { 0.0f, worldAngle + spinAngle, 0.0f };
}

void Sword::BackGroundDraw()
{
}

void Sword::Draw()
{
	multiCollider_->Draw();
	object3d_->Draw();
}

void Sword::ForeGroundDraw()
{
}

void Sword::ParticleDraw()
{
}

void Sword::AnimationDraw()
{
}

void Sword::OnCollision()
{
	isHit_ = true;
}
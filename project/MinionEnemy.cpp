#include "MinionEnemy.h"
#include "engine/TimeManager.h"
#include <cmath>

static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

static Vector3 NormalizeSafe(const Vector3& v)
{
	float len = Length(v);
	if (len < 1e-6f) return { 0,0,1 };
	return v / len;
}

MinionEnemy::MinionEnemy(BaseScene* scene)
	: ObjectBase(scene)
{
}

void MinionEnemy::InitializeMinion(const Vector3& spawnPos)
{
	ModelManager::GetInstance()->LoadModel("minion.obj");

	object3d_->Initialize();
	object3d_->SetModel("minion.obj");

	// 立ち位置を記憶（モデルの足元が地面に来るよう少し持ち上げる）
	surfacePos_ = spawnPos;
	surfacePos_.y += groundOffsetY_;
	groundY_ = surfacePos_.y;

	// 地面の下からスタート
	transform.translate = surfacePos_;
	transform.translate.y -= spawnDepth_;
	transform.rotate = { 0,0,0 };
	transform.scale = { 1,1,1 };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	phase_ = Phase::Spawn;
	isDead_ = false;
	attackActive_ = false;
	actedThisRound_ = false;
	hp_ = kMaxHP_;
	shockwaveTimer_ = 0.0f;

	// コライダー（Sphere）
	multiCollider_->Clear();

	Sphere sp{};
	sp.center = transform.translate;
	sp.radius = radius_;
	multiCollider_->AddSphere(sp);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });
}

void MinionEnemy::BeginAttack(const Vector3& targetPos)
{
	// 待機中以外から呼ばれても無視する
	if (phase_ != Phase::Idle) return;

	// 突進先を確定（XZのみ採用。高さは自分の地面に合わせる）
	chargeTargetXZ_ = { targetPos.x, groundY_, targetPos.z };
	transform.translate.y = groundY_; // 待機のホバー揺れ位置から地面へ戻して突進開始
	phase_ = Phase::Charge;
	attackActive_ = true;
}

void MinionEnemy::Update()
{
	if (isDead_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();

	switch (phase_)
	{
	case Phase::Spawn:
	{
		// 地面の下から上昇
		transform.translate.y += spawnRiseSpeed_ * dt;

		// 地面の高さに到達したら待機へ
		if (transform.translate.y >= surfacePos_.y) {
			transform.translate.y = surfacePos_.y;
			phase_ = Phase::Idle;
		}
		break;
	}

	case Phase::Idle:
	{
		// 生きた待機：完全静止にせず、ゆるくホバー上下動しつつプレイヤーを向く。
		// Enemy の BeginAttack を待つ点は同じ。
		idleTime_ += dt;

		// ホバー上下動（groundY_ から上方向にだけ揺れる）
		const float bob = (1.0f - std::cos(idleTime_ * idleBobSpeed_)) * 0.5f * idleBobAmplitude_;
		transform.translate.y = groundY_ + bob;

		// ゆっくりプレイヤーの方を向く
		if (playerTarget_) {
			Vector3 toPlayer = playerTarget_->translate - transform.translate;
			toPlayer.y = 0.0f;
			if (Length(toPlayer) > 1e-4f) {
				Vector3 dir = NormalizeSafe(toPlayer);
				const float desiredYaw = std::atan2(dir.x, dir.z);
				transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, idleTurnLerp_);
			}
		}
		break;
	}

	case Phase::Charge:
	{
		// 取得済みのプレイヤー位置へ突進
		Vector3 toTarget = chargeTargetXZ_ - transform.translate;
		toTarget.y = 0.0f;
		float dist = Length(toTarget);

		if (dist <= reachThreshold_) {
			// 到達 → 一旦止まって上昇へ
			transform.translate.x = chargeTargetXZ_.x;
			transform.translate.z = chargeTargetXZ_.z;
			phase_ = Phase::Rise;
		}
		else {
			Vector3 dir = NormalizeSafe(toTarget);

			// 進行方向を向く
			float desiredYaw = std::atan2(dir.x, dir.z);
			transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_);

			// 行き過ぎないようにクランプ
			float step = chargeSpeed_ * dt;
			if (step > dist) step = dist;
			transform.translate.x += dir.x * step;
			transform.translate.z += dir.z * step;
		}
		break;
	}

	case Phase::Rise:
	{
		// 突進先でY座標上に上昇
		transform.translate.y += liftSpeed_ * dt;
		if (transform.translate.y >= groundY_ + riseHeight_) {
			transform.translate.y = groundY_ + riseHeight_;
			phase_ = Phase::SpinTop;
			spinAccumulated_ = 0.0f;
		}
		break;
	}

	case Phase::SpinTop:
	{
		// てっぺんで停止したまま高速回転（spinTurns_ 周）
		float d = spinSpeed_ * dt;
		transform.rotate.y += d;
		spinAccumulated_ += d;
		if (spinAccumulated_ >= spinTurns_ * 2.0f * 3.1415926535f) {
			phase_ = Phase::Slam;
		}
		break;
	}

	case Phase::Slam:
	{
		// 急降下（回転はしない）
		transform.translate.y -= slamSpeed_ * dt;
		if (transform.translate.y <= groundY_) {
			transform.translate.y = groundY_;
			transform.scale = impactScale_; // 着地の衝撃で潰れる（Recoverで戻す）
			phase_ = Phase::Shockwave;
			shockwaveTimer_ = 0.0f;
		}
		break;
	}

	case Phase::Shockwave:
	{
		// 着地点に範囲ダメージ（下のコライダー更新で半径を拡大）
		shockwaveTimer_ += dt;
		if (shockwaveTimer_ >= shockwaveDuration_) {
			// 衝撃波おわり → 硬直(Recover)へ。
			// attackActive_ をここで false にするので、コーディネーターは
			// 従来どおりこのタイミングで次の雑魚敵を選ぶ（突進間隔は変わらない）。
			phase_ = Phase::Recover;
			recoverTimer_ = 0.0f;
			attackActive_ = false;
		}
		break;
	}

	case Phase::Recover:
	{
		// 急降下後の硬直：無防備で動けない＝プレイヤーの反撃チャンス。
		// 潰れスケールの戻りは硬直より短い scaleRecoverDuration_ で素早く立ち直らせる。
		recoverTimer_ += dt;
		float t = recoverTimer_ / scaleRecoverDuration_;
		if (t > 1.0f) t = 1.0f;
		transform.scale.x = impactScale_.x + (1.0f - impactScale_.x) * t;
		transform.scale.y = impactScale_.y + (1.0f - impactScale_.y) * t;
		transform.scale.z = impactScale_.z + (1.0f - impactScale_.z) * t;

		if (recoverTimer_ >= recoverDuration_) {
			transform.scale = { 1.0f, 1.0f, 1.0f };
			idleTime_ = 0.0f;       // 待機演出を最初から
			phase_ = Phase::Idle;
		}
		break;
	}
	}

	// コライダー更新（衝撃波中のみ半径を拡大して範囲ダメージにする）
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = transform.translate;
	sp.radius = (phase_ == Phase::Shockwave) ? shockwaveRadius_ : radius_;

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();
}

void MinionEnemy::Draw()
{
	multiCollider_->Draw();

	object3d_->Draw();
}

void MinionEnemy::OnCollision(const CollisionInfo& info)
{
	auto other = static_cast<CollisionTypeIdDef>(info.otherType);

	if (other == CollisionTypeIdDef::kPlayerWeapon ||
		other == CollisionTypeIdDef::kPlayerAttack ||
		other == CollisionTypeIdDef::PlayerBullet)
	{
		hp_ -= 1;
		if (hp_ <= 0) {
			isDead_ = true;
		}
	}
}

void MinionEnemy::SetCamera(Camera* camera)
{
	ObjectBase::SetCamera(camera);
}

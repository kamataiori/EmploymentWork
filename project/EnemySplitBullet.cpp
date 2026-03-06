#include "EnemySplitBullet.h"
#include "engine/TimeManager.h"
#include <cmath>

static Vector3 NormalizeSafe(const Vector3& v)
{
	float len = Length(v);
	if (len < 1e-6f) return { 0,0,1 };
	return v / len;
}

void EnemySplitBullet::InitializeBurst(
	const Vector3& startPos,
	const Vector3& lockPlayerPos,
	float riseHeight,
	float riseSpeed,
	float splitRadius,
	float shotSpeed
)
{
	startPos_ = startPos;
	lockPlayerPos_ = lockPlayerPos;

	riseHeight_ = riseHeight;
	riseSpeed_ = riseSpeed;
	splitRadius_ = splitRadius;
	shotSpeed_ = shotSpeed;

	transform.translate = startPos_;
	transform.rotate = { 0,0,0 };
	transform.scale = { 1,1,1 };

	// collider（Sphere1個）
	multiCollider_->Clear();

	Sphere sp{};
	sp.center = transform.translate;
	sp.radius = radius_;
	multiCollider_->AddSphere(sp);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyBullet));
	multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });

	phase_ = Phase::Rise;
	isDead_ = false;
	phaseTimer_ = 0.0f;

	//==========================
	// Particle 初期化
	//==========================
	particleSystem_ = std::make_unique<ParticleManager>();
	particleSystem_->Initialize(VertexDataType::Plane);

	particleSystem_->LoadAllPresets();
	particleSystem_->LoadAllSystems();

	if (camera_) {
		particleSystem_->SetCamera(camera_);
	}

	particleTransform_ = transform;

	// 初回Emit：エミッターを作成
	particleTransform_.translate = transform.translate;
	particleSystem_->EmitSystemByName(particleSystemName_, particleTransform_);

	emitTimer_ = 0.0f;
}

void EnemySplitBullet::Fire()
{
	// ★ SplitWait 状態のときだけ発射に移行する
	if (phase_ == Phase::SplitWait)
	{
		phase_ = Phase::Shot;
	}
}

void EnemySplitBullet::Update()
{
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	//==========================
	// フェーズごとの挙動
	//==========================
	if (phase_ == Phase::Rise)
	{
		transform.translate.y += riseSpeed_ * dt;

		if (transform.translate.y >= startPos_.y + riseHeight_)
		{
			transform.translate.y = startPos_.y + riseHeight_;

			// 分裂先の目標位置を計算
			Vector3 base = transform.translate;
			switch (index_)
			{
			case 0: splitTargetPos_ = base + Vector3{ +splitRadius_, 0.0f, 0.0f }; break;
			case 1: splitTargetPos_ = base + Vector3{ -splitRadius_, 0.0f, 0.0f }; break;
			case 2: splitTargetPos_ = base + Vector3{ 0.0f, 0.0f, +splitRadius_ }; break;
			case 3: splitTargetPos_ = base + Vector3{ 0.0f, 0.0f, -splitRadius_ }; break;
			default: splitTargetPos_ = base; break;
			}

			// 上昇後、少し止まる
			phase_ = Phase::RiseWait;
			phaseTimer_ = 0.0f;
		}
	}
	else if (phase_ == Phase::RiseWait)
	{
		// 上昇後の待機
		phaseTimer_ += dt;
		if (phaseTimer_ >= riseWaitDuration_)
		{
			phase_ = Phase::SplitMove;
		}
	}
	else if (phase_ == Phase::SplitMove)
	{
		// 四方向に分裂移動
		Vector3 to = splitTargetPos_ - transform.translate;
		float d = Length(to);

		if (d < 0.05f)
		{
			transform.translate = splitTargetPos_;

			// プレイヤーへの方向をロック
			Vector3 toP = lockPlayerPos_ - transform.translate;
			shotDir_ = NormalizeSafe(toP);

			// ★ ここでShotに行かず、発射許可を待つ
			phase_ = Phase::SplitWait;
		}
		else
		{
			Vector3 dir = NormalizeSafe(to);
			float step = splitMoveSpeed_ * dt;
			if (step > d) step = d;
			transform.translate += dir * step;
		}
	}
	else if (phase_ == Phase::SplitWait)
	{
		// ★ 何もしない。Enemy側からFire()が呼ばれるのを待つだけ
	}
	else // Shot
	{
		transform.translate += shotDir_ * (shotSpeed_ * dt);

		if (Length(transform.translate - startPos_) > 200.0f)
		{
			isDead_ = true;
		}
	}

	// collider 更新
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = transform.translate;
	sp.radius = radius_;

	//==========================
	// Particle 更新
	//==========================
	if (particleSystem_)
	{
		particleTransform_ = transform;
		particleSystem_->EmitSystemByName(particleSystemName_, particleTransform_);
		particleSystem_->Update();
	}

#ifdef USE_IMGUI
	ImGui::Begin("SplitBullet Debug");

	ImGui::Text("=== Bullet [%d] ===", index_);
	const char* phaseNames[] = { "Rise", "RiseWait", "SplitMove", "SplitWait", "Shot" };
	ImGui::Text("Phase: %s", phaseNames[static_cast<int>(phase_)]);
	ImGui::Text("phaseTimer: %.3f", phaseTimer_);
	ImGui::DragFloat3("Bullet translate", &transform.translate.x, 0.01f);

	ImGui::Separator();
	ImGui::DragFloat3("Particle translate", &particleTransform_.translate.x, 0.01f);

	ImGui::Separator();
	ImGui::Text("SystemName: %s", particleSystemName_.c_str());
	ImGui::Text("particleSystem_: %s", particleSystem_ ? "exists" : "NULL");
	ImGui::Text("isDead: %s", isDead_ ? "true" : "false");

	ParticleSystem* sys = particleSystem_->FindSystem(particleSystemName_);
	if (sys) {
		ImGui::Text("System found: YES");
		ImGui::Text("Emitter count: %d", (int)sys->GetEmitters().size());
	}
	else {
		ImGui::Text("System found: NO");
	}

	ImGui::End();
#endif // USE_IMGUI
}

void EnemySplitBullet::Draw()
{
	multiCollider_->Draw();
}

void EnemySplitBullet::ParticleDraw()
{
	if (particleSystem_) {
		particleSystem_->Draw();
	}
}

void EnemySplitBullet::OnCollision(const CollisionInfo& info)
{
	auto other = static_cast<CollisionTypeIdDef>(info.otherType);

	if (other == CollisionTypeIdDef::kPlayer ||
		other == CollisionTypeIdDef::kPlayerWeapon ||
		other == CollisionTypeIdDef::kPlayerAttack ||
		other == CollisionTypeIdDef::PlayerBullet)
	{
		isDead_ = true;
	}
}

void EnemySplitBullet::SetCamera(Camera* camera)
{
	ObjectBase::SetCamera(camera);
	if (particleSystem_) {
		particleSystem_->SetCamera(camera);
	}
}
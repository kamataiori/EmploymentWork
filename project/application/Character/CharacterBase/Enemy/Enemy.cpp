#include "Enemy.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "EnemyDropBullet.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>
#include "engine/UI/UIManager.h"
#include <engine/UI/UIHpBar.h>
#include <EnemySplitBullet.h>
#include "State/EnemyStateManager.h"

// Yaw(=Y回転)から OBB の3軸を作る簡易ヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f,  0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

// 角度差分を [-π, π] に折りたたむ
static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

// 角度の線形補間（ラップ考慮）
static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

Enemy::Enemy(BaseScene* baseScene_)
	: ObjectBase(baseScene_)
{
}

Enemy::~Enemy() = default;

void Enemy::Initialize()
{
	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");

	object3d_->Initialize();
	object3d_->SetModel("Skeleton.gltf");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f,30.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 3.0f, 3.0f, 3.0f };

	// 初期位置を保存
	homePosition_ = transform.translate;

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->SetAnimation(animation_.Idle);

	// -------------------------
	// AI(BT) 初期化
	// -------------------------
	aiController_ = std::make_unique<EnemyAIController>();
	aiController_->Initialize(this);

	aiController_->SetTargetGetter([this]() -> const Transform* {
		return this->GetTargetTransform();
		});

	// ---- OBB コライダー初期化 ----
	colliderCenter_ = transform.translate + colliderOffset_;

	OBB obb{};
	obb.center = colliderCenter_;
	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];
	obb.size = obbSize_;

	Shape first{};
	first.kind = ShapeKind::OBB;
	first.obb = obb;

	*multiCollider_ = MultiCollider(first);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

	// === HPバー初期化 ===

	uiManager_ = std::make_unique<UIManager>();
	const float winW = 1280.0f;
	const float barW = 420.0f;
	const float barH = 20.0f;
	const float top = 20.0f;
	UIHpBar::CreateDesc desc{};
	desc.bgTexPath = "Resources/hp.png";
	desc.fillTexPath = "Resources/hp.png";
	desc.pos = { winW * 0.5f - barW * 0.5f, top };
	desc.size = { barW, barH };
	desc.anchor = { 0.0f, 0.5f };
	desc.layer = 100;
	desc.bgColor = { 0.2f, 0.2f, 0.2f, 0.8f };
	desc.fillColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	uiManager_->Add(UIHpBar::Create(desc));

	// -------------------------
	// 死亡エフェクト用パーティクル
	// -------------------------
	deathSystem_ = std::make_unique<ParticleManager>();
	deathSystem_->Initialize(VertexDataType::Plane);

	deathSystem_->LoadAllPresets();
	deathSystem_->LoadAllSystems();

	deathParticleTransform_ = transform;

	// ★ ステートマネージャ初期化
	stateManager_ = std::make_unique<EnemyStateManager>();
}

void Enemy::Update()
{
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	if (!isDead_) {

		if (!isDead_) {
			if (aiController_) {
				aiController_->Update(dt);
			}

			// ★ ステートの実行（BT が選んだ攻撃をここで毎フレーム進める）
			if (stateManager_) {
				stateManager_->Update(this, dt);
			}

			for (auto it = dropBullets_.begin(); it != dropBullets_.end(); )
			{
				(*it)->Update();
				if ((*it)->IsDead()) {
					it = dropBullets_.erase(it);
				}
				else {
					++it;
				}
			}

			for (auto it = splitBullets_.begin(); it != splitBullets_.end(); )
			{
				(*it)->Update();
				if ((*it)->IsDead()) {
					it = splitBullets_.erase(it);
				}
				else {
					++it;
				}
			}

			// 分裂弾の順次発射制御
			UpdateSplitBulletFiring(dt);
		}

	}

	// 当たり判定中心を更新
	colliderCenter_ = transform.translate + colliderOffset_;

	// コライダー更新
	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	obb.size = { obbSize_.x, obbSize_.y, obbSize_.z };

	// HitReact の終了管理
	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= dt;
		if (hitReactTimer_ <= 0.0f) {
			SetAnimationIfChanged(animation_.Idle);
			hitReactTimer_ = 0.0f;
		}
	}

	// オブジェクト更新処理
	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

#ifdef USE_IMGUI
	ImGui::Begin("Enemy");
	ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
	if (stateManager_) {
		ImGui::Text("ActionState: %s", stateManager_->GetCurrentStateName());
		ImGui::Text("State Finished: %s", stateManager_->IsFinished() ? "true" : "false");
	}
	ImGui::End();
#endif // USE_IMGUI

	// ======== 死亡演出 ========
	if (isDead_) {
		deathTimer_ += dt;

		const float kShrinkDelay = 0.8f;
		const float kShrinkDuration = 1.0f;

		if (deathTimer_ >= kShrinkDelay) {
			float t = (deathTimer_ - kShrinkDelay) / kShrinkDuration;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;

			transform.scale.x = deathStartScale_.x * (1.0f - t);
			transform.scale.y = deathStartScale_.y * (1.0f - t);
			transform.scale.z = deathStartScale_.z * (1.0f - t);

			if (!hasSpawnedExplosion_ && t >= 1.0f) {
				explosionPos = transform.translate;

				deathParticleTransform_.translate = explosionPos;
				deathParticleTransform_.translate.y += 3.5f;
				deathSystem_->EmitSystemByName("ADE", deathParticleTransform_);

				hasSpawnedExplosion_ = true;
			}
		}

#ifdef USE_IMGUI
		ImGui::Begin("explosionPos");
		ImGui::DragFloat3("translate", &explosionPos.x, 0.01f);
		ImGui::End();
#endif // USE_IMGUI

		if (transform.scale.x < 0.0f) transform.scale.x = 0.0f;
		if (transform.scale.y < 0.0f) transform.scale.y = 0.0f;
		if (transform.scale.z < 0.0f) transform.scale.z = 0.0f;

		if (deathSystem_) {
			deathSystem_->Update();
		}

		if (deathTimer_ >= kDeathToTitleDelay_) {
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
		return;
	}

	// HP を UI に反映
	UIElement::UIData data{};
	data.hp = static_cast<float>(hp_);
	data.maxHp = static_cast<float>(kMaxHP_);

	uiManager_->ApplyDataToAll(data);
	uiManager_->Update();
}

void Enemy::UpdateSplitBulletFiring(float dt)
{
	if (splitBullets_.empty()) {
		splitFireActive_ = false;
		return;
	}

	if (!splitFireActive_)
	{
		bool allReady = true;
		for (auto& b : splitBullets_) {
			if (!b->IsReadyToShot()) {
				allReady = false;
				break;
			}
		}

		if (allReady) {
			splitFireActive_ = true;
			splitFireTimer_ = 0.0f;

			for (auto& b : splitBullets_) {
				if (b->IsReadyToShot()) {
					b->Fire();
					break;
				}
			}
		}
		return;
	}

	splitFireTimer_ += dt;
	if (splitFireTimer_ >= splitFireInterval_)
	{
		splitFireTimer_ = 0.0f;

		for (auto& b : splitBullets_) {
			if (b->IsReadyToShot()) {
				b->Fire();
				break;
			}
		}

		bool anyWaiting = false;
		for (auto& b : splitBullets_) {
			if (b->IsReadyToShot()) {
				anyWaiting = true;
				break;
			}
		}
		if (!anyWaiting) {
			splitFireActive_ = false;
		}
	}
}

void Enemy::BackGroundDraw()
{
}

void Enemy::Draw()
{
	multiCollider_->Draw();

	for (auto& b : dropBullets_) {
		b->Draw();
	}

	for (auto& b : splitBullets_) {
		b->Draw();
	}
}

void Enemy::ForeGroundDraw()
{
	if (uiManager_) {
		uiManager_->Draw();
	}
}

void Enemy::AnimationDraw()
{
	object3d_->Draw();
}

void Enemy::ParticleDraw()
{
	if (isDead_) {
		deathSystem_->Draw();
	}

	for (auto& b : splitBullets_) {
		b->ParticleDraw();
	}
}

void Enemy::OnCollision()
{
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	if (hp_ <= 0 && !isDead_) {
		object3d_->SetAnimationOneShot(animation_.Death);
		hitReactTimer_ = 0.0f;

		isDead_ = true;
		deathTimer_ = 0.0f;

		deathStartScale_ = transform.scale;
		hasSpawnedExplosion_ = false;

		return;
	}

	if (!isDead_) {
		SetAnimationIfChanged(animation_.HitReact);
		hitReactTimer_ = kHitReactDuration_;
	}
}

void Enemy::OnCollision(const CollisionInfo& info)
{
	const auto otherType = static_cast<CollisionTypeIdDef>(info.otherType);

	if (otherType == CollisionTypeIdDef::kPlayerWeapon)
	{
		OnCollision();
		return;
	}
}

void Enemy::SetCamera(Camera* camera)
{
	ObjectBase::SetCamera(camera);
	deathSystem_->SetCamera(camera);
}

void Enemy::SetAnimationIfChanged(const std::string& name)
{
	if (currentAnimationName_ != name) {
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}
}

void Enemy::SpawnSplitBurstToPlayer(const Vector3& playerPos)
{
	Vector3 start = transform.translate;
	start.y += 2.0f;

	const float riseHeight = 12.0f;
	const float riseSpeed = 18.0f;
	const float splitRadius = 4.0f;
	const float shotSpeed = 28.0f;

	for (int i = 0; i < 4; ++i)
	{
		auto b = std::make_unique<EnemySplitBullet>(GetBaseScene());
		b->SetCamera(GetCamera());
		b->SetIndex(i);
		b->InitializeBurst(start, playerPos, riseHeight, riseSpeed, splitRadius, shotSpeed);

		splitBullets_.push_back(std::move(b));
	}

	splitFireActive_ = false;
	splitFireTimer_ = 0.0f;
}
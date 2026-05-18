#include "Enemy.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "EnemyDropBullet.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>
#include "engine/UI/UIManager.h"
#include <engine/UI/UIHpBar.h>
#include <EnemySplitBullet.h>
#include <MinionEnemy.h>
#include "State/EnemyStateManager.h"

// Yaw(=Y回転)から OBB の3軸を作る簡易ヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	outAxes[0] = { c, 0.0f, -s };
	outAxes[1] = { 0.0f, 1.0f,  0.0f };
	outAxes[2] = { s, 0.0f,  c };
}

static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

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
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");

	object3d_->Initialize();
	object3d_->SetModel("Skeleton.gltf");

	transform.translate = { 0.0f, 0.0f,30.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 3.0f, 3.0f, 3.0f };

	homePosition_ = transform.translate;

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->SetAnimation(animation_.Idle);

	aiController_ = std::make_unique<EnemyAIController>();
	aiController_->Initialize(this);

	aiController_->SetTargetGetter([this]() -> const Transform* {
		return this->GetTargetTransform();
		});

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

	deathSystem_ = std::make_unique<ParticleManager>();
	deathSystem_->Initialize(VertexDataType::Plane);
	deathSystem_->LoadAllPresets();
	deathSystem_->LoadAllSystems();
	deathParticleTransform_ = transform;

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

			// ステートの実行
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

			// ★ 雑魚敵の更新
			for (auto it = minions_.begin(); it != minions_.end(); )
			{
				(*it)->Update();
				if ((*it)->IsDead()) {
					it = minions_.erase(it);
				}
				else {
					++it;
				}
			}

			UpdateSplitBulletFiring(dt);
		}

	}

	colliderCenter_ = transform.translate + colliderOffset_;

	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	obb.size = { obbSize_.x, obbSize_.y, obbSize_.z };

	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= dt;
		if (hitReactTimer_ <= 0.0f) {
			SetAnimationIfChanged(animation_.Idle);
			hitReactTimer_ = 0.0f;
		}
	}

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

#ifdef USE_IMGUI
	/*ImGui::Begin("Enemy");
	ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
	if (stateManager_) {
		ImGui::Text("ActionState: %s", stateManager_->GetCurrentStateName());
		ImGui::Text("State Finished: %s", stateManager_->IsFinished() ? "true" : "false");
	}
	ImGui::Text("Minion count: %d", (int)minions_.size());
	ImGui::End();*/
#endif

	// 死亡演出
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
		/*ImGui::Begin("explosionPos");
		ImGui::DragFloat3("translate", &explosionPos.x, 0.01f);
		ImGui::End();*/
#endif

		if (transform.scale.x < 0.0f) transform.scale.x = 0.0f;
		if (transform.scale.y < 0.0f) transform.scale.y = 0.0f;
		if (transform.scale.z < 0.0f) transform.scale.z = 0.0f;

		// 生存中もパーティクル更新（ヒットエフェクト用）
		if (!isDead_ && deathSystem_) {
			deathSystem_->Update();
		}

		if (deathTimer_ >= kDeathToTitleDelay_) {
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
		return;
	}

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

	if (!splitFireActive_) {
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
	if (splitFireTimer_ >= splitFireInterval_) {
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
	// 雑魚敵描画
	for (auto& m : minions_) {
		m->Draw();
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
	// 生存中・死亡時どちらでもパーティクル描画
	if (deathSystem_) {
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

		// 被弾時のヒットパーティクルを発生
		if (deathSystem_) {
			Transform hitParticleTransform = transform;
			hitParticleTransform.translate.y += 3.5f; // 胴体あたりの高さ
			deathSystem_->EmitSystemByName("ADE", hitParticleTransform);
		}
	}
}

void Enemy::OnCollision(const CollisionInfo& info)
{
	const auto otherType = static_cast<CollisionTypeIdDef>(info.otherType);
	if (otherType == CollisionTypeIdDef::kPlayerWeapon) {
		OnCollision();
		return;
	}
}

void Enemy::UpdateVisual()
{
	// AI・弾・ステートを動かさず、見た目（object3d_）だけ更新する
	// イントロ演出中に呼ぶ専用メソッド
	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();
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

	const float riseHeight = 10.0f;
	const float riseSpeed = 40.0f;
	const float splitRadius = 4.0f;
	const float shotSpeed = 60.0f;

	for (int i = 0; i < 4; ++i) {
		auto b = std::make_unique<EnemySplitBullet>(GetBaseScene());
		b->SetCamera(GetCamera());
		b->SetIndex(i);
		b->InitializeBurst(start, playerPos, riseHeight, riseSpeed, splitRadius, shotSpeed);
		splitBullets_.push_back(std::move(b));
	}

	splitFireActive_ = false;
	splitFireTimer_ = 0.0f;
}

// 怒り時：分裂弾を8発発射（通常の2倍）
void Enemy::SpawnSplitBurstAngry(const Vector3& playerPos)
{
	Vector3 start = transform.translate;
	start.y += 1.0f;

	// 通常より速く・広く・多い
	const float riseHeight = 10.0f;
	const float riseSpeed = 50.0f;
	const float splitRadius = 6.0f;
	const float shotSpeed = 70.0f;
	const int   bulletCount = 8;    // 通常4発 → 8発

	for (int i = 0; i < bulletCount; ++i) {
		auto b = std::make_unique<EnemySplitBullet>(GetBaseScene());
		b->SetCamera(GetCamera());
		b->SetIndex(i % 4);  // index は 0〜3 でループ
		b->InitializeBurst(start, playerPos, riseHeight, riseSpeed, splitRadius, shotSpeed);
		splitBullets_.push_back(std::move(b));
	}

	splitFireActive_ = false;
	splitFireTimer_ = 0.0f;
}

// 大弾を生成（ThrowBigBulletState から呼ばれる）
void Enemy::SpawnBigBullet(const Vector3& targetPos)
{
	Vector3 start = transform.translate;
	start.y += 2.0f;

	// EnemyDropBullet を大きめのパラメーターで生成
	auto b = std::make_unique<EnemyDropBullet>(GetBaseScene());
	b->SetCamera(GetCamera());
	b->Initialize(start, targetPos);
	// radius を大きくするため BigBullet 用スケールを設定
	Transform bt = b->GetTransform();
	bt.scale = { 3.0f, 3.0f, 3.0f };  // 通常弾の3倍サイズ
	b->SetTransform(bt);
	dropBullets_.push_back(std::move(b));
}

// 雑魚敵を1体召喚
void Enemy::SpawnMinion(const Vector3& spawnPos)
{
	auto m = std::make_unique<MinionEnemy>(GetBaseScene());
	// InitializeMinion 内の object3d_->Initialize() がカメラを上書きするため、
	// SetCamera は必ず InitializeMinion の後に呼ぶ
	m->InitializeMinion(spawnPos, target_);
	m->SetCamera(GetCamera());
	minions_.push_back(std::move(m));
}
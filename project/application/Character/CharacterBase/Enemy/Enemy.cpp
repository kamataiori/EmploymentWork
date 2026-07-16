#include "Enemy.h"
#include "engine/TimeManager.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "EnemyDropBullet.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>
#include "engine/UI/UIManager.h"
#include <engine/UI/UIHpBar.h>
#include "engine/UI/IDamagePopupSink.h"
#include <EnemySplitBullet.h>
#include <MinionEnemy.h>
#include "State/EnemyStateManager.h"
#include "engine/3d/Camera/CameraEffectController.h"
#include <random>
#include <vector>
#include <algorithm>

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

	transform.translate = { 0.0f, 0.0f,280.0f };
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
	// 相手の種別を見て被弾を判定するため Ex 版で登録する
	// （SetHitCallback だと種別情報が捨てられ、何に当たっても無条件で被弾する）
	multiCollider_->SetHitCallbackEx(
		[this](const CollisionInfo& info) { this->OnCollision(info); });

	// --- 押し戻し専用の物理プロキシ（ステージとだけ当たる Capsule） ---
	// ヒット判定用の本体OBBとは分離し、押し戻し(physics)だけを担当する。
	{
		bodyProxy_ = std::make_unique<MultiCollider>();
		const float radius = std::min(obbSize_.x, obbSize_.z); // 胴体の水平半径
		const float halfSeg = (std::max)(0.0f, obbSize_.y - radius); // 芯線の半長
		Capsule cap{};
		cap.start = colliderCenter_ - Vector3{ 0.0f, halfSeg, 0.0f };
		cap.end = colliderCenter_ + Vector3{ 0.0f, halfSeg, 0.0f };
		cap.radius = radius;
		bodyProxy_->AddCapsule(cap);
		bodyProxy_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemyBody));
		bodyProxy_->SetResponse(CollisionResponse::Blocking);
		bodyProxy_->SetMovable(true);
		bodyProxy_->SetPushOutCallback([this](const Vector3& mtv) {
			transform.translate += mtv;
			colliderCenter_ = transform.translate + colliderOffset_;
			object3d_->SetTranslate(transform.translate);
			// Update() はスキニングのDispatchとリソースバリアを含み、1フレームに1回しか呼べない。
			// 押し戻しは Update() の後に複数回来るので、行列だけ更新する。
			object3d_->UpdateTransform();
		});
	}

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

	// 回転薙ぎ払いの攻撃判定（球）。本体とは別コライダー
	{
		Sphere sp{};
		sp.center = transform.translate;
		sp.radius = 0.0f;

		Shape sh{};
		sh.kind = ShapeKind::Sphere;
		sh.sphere = sp;

		spinHitbox_ = std::make_unique<MultiCollider>(sh);
		spinHitbox_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyAreaAttack));
	}
}

void Enemy::Update()
{
	// 休眠中はAIも弾も見た目も動かさない（スキニングのバリアも発行しない）
	if (!active_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// 登場の落下中。AI・弾・雑魚は動かさず、落ちることだけに専念させる。
	// この下のコライダー同期と見た目更新は通常どおり走るので、
	// 落ちてくる本体に当たり判定も描画もついてくる。
	if (dropping_) {
		UpdateDropIn(dt);
	}

	if (!isDead_ && !dropping_) {

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

			// 雑魚敵の更新
			for (auto it = minions_.begin(); it != minions_.end(); )
			{
				(*it)->SetPlayerTarget(target_); // 待機中の向き追従用
				(*it)->Update();
				if ((*it)->IsDead()) {
					if (it->get() == activeMinion_) {
						activeMinion_ = nullptr;
					}
					it = minions_.erase(it);
				}
				else {
					++it;
				}
			}

			// 雑魚敵を1体ずつ順番に突進させる
			UpdateMinionCoordinator();

			UpdateSplitBulletFiring(dt);
		}

	}

	colliderCenter_ = transform.translate + colliderOffset_;

	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	// 押し戻しプロキシ(Capsule)も胴体位置へ同期
	if (bodyProxy_ && !bodyProxy_->GetShapes().empty()) {
		const float radius = std::min(obbSize_.x, obbSize_.z);
		const float halfSeg = (std::max)(0.0f, obbSize_.y - radius);
		Capsule& cap = bodyProxy_->MutableCapsule(0);
		cap.start = colliderCenter_ - Vector3{ 0.0f, halfSeg, 0.0f };
		cap.end = colliderCenter_ + Vector3{ 0.0f, halfSeg, 0.0f };
		cap.radius = radius;
	}

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	obb.size = { obbSize_.x, obbSize_.y, obbSize_.z };

	// 回転薙ぎ払いの攻撃判定をボスへ追従させる
	UpdateSpinHitbox();

	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= dt;
		if (hitReactTimer_ <= 0.0f) {
			SetAnimationIfChanged(animation_.Idle);
			hitReactTimer_ = 0.0f;
		}
	}

	// 被弾リアクション層：AI が決めた transform に“視覚オフセット”を上乗せする。
	// transform 本体は触らないので、AI／当たり判定はリアクションの影響を受けない。
	Vector3 visualPos = transform.translate;
	Vector3 visualScale = transform.scale;
	if (hitFxTimer_ > 0.0f) {
		// ヒットストップで世界が止まっていても見せたいので、実時間で減衰させる
		hitFxTimer_ -= TimeManager::GetInstance()->GetUnscaledDeltaTime();
		if (hitFxTimer_ < 0.0f) hitFxTimer_ = 0.0f;

		const float remain = hitFxTimer_ / kHitFxDuration_;   // 1→0（被弾直後が最大、終わりに向け減衰）
		const float elapsed = kHitFxDuration_ - hitFxTimer_;

		// 小刻みな震え（X/Y/Z で周波数・位相をずらす）
		const float amp = kHitFxShakeAmplitude_ * remain;
		visualPos.x += std::sin(elapsed * kHitFxShakeFreq_) * amp;
		visualPos.y += std::sin(elapsed * kHitFxShakeFreq_ * 1.3f + 0.5f) * amp * 0.6f;
		visualPos.z += std::cos(elapsed * kHitFxShakeFreq_ * 1.1f) * amp;

		// 小さな後退（プレイヤーと反対へ怯み、減衰でバネのように戻る）
		const float recoil = kHitFxRecoilDistance_ * remain;
		visualPos.x += hitFxRecoilDir_.x * recoil;
		visualPos.z += hitFxRecoilDir_.z * recoil;

		// つぶれ（Yを縮め、XZを少し広げる）。被弾直後が最大で通常へ戻る。
		const float s = kHitFxSquash_ * remain;
		visualScale.y *= (1.0f - s);
		visualScale.x *= (1.0f + s * 0.5f);
		visualScale.z *= (1.0f + s * 0.5f);
	}

	object3d_->SetTranslate(visualPos);
	object3d_->SetScale(visualScale);
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

	// パーティクル更新（ヒット/死亡/着地エフェクト共通）
	// 生存中も毎フレーム呼ばないとアニメーションしないため、ここで一括更新する
	if (deathSystem_) {
		deathSystem_->Update();
	}

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
	if (!active_) return;

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
	if (!active_) return;

	if (uiManager_) {
		uiManager_->Draw();
	}
	// 雑魚敵の頭上HPバーは個体ごとに前景パスで描画する
	for (auto& m : minions_) {
		m->ForeGroundDraw();
	}
}

void Enemy::AnimationDraw()
{
	if (!active_) return;

	object3d_->Draw();
}

void Enemy::ParticleDraw()
{
	if (!active_) return;

	// 生存中・死亡時どちらでもパーティクル描画
	if (deathSystem_) {
		deathSystem_->Draw();
	}
	for (auto& b : splitBullets_) {
		b->ParticleDraw();
	}
	// 雑魚敵の撃破時の爆発パーティクルを描画する
	for (auto& m : minions_) {
		m->ParticleDraw();
	}
}

void Enemy::OnCollision()
{
	// 通常の被弾（プレイヤー武器との衝突）は既定ダメージで処理する。
	ApplyDamage(kDamagePerHit_);
}

void Enemy::ApplyDamage(int amount)
{
	if (isDead_) return;

	// 与ダメージ量を敵の右上にポップアップ表示（敵への加害は全てプレイヤー由来）
	if (damageSink_ && amount > 0) {
		damageSink_->SpawnDamage(GetTargetCenter(), amount);
	}

	// 被弾した瞬間に火花を飛び散らせる（撃破ヒットでも出るよう、死亡判定の前に出す）
	// EmitByPresetName は描画されないため、描画される EmitPreset 経路で出す。
	if (deathSystem_) {
		Transform spark = transform;
		spark.translate.y += kHitSparkOffsetY_;
		deathSystem_->EmitPreset(kHitSparkPreset_, spark);
	}

	hp_ -= amount;
	if (hp_ < 0) hp_ = 0;

	if (hp_ <= 0) {
		object3d_->SetAnimationOneShot(animation_.Death);
		hitReactTimer_ = 0.0f;
		isDead_ = true;
		deathTimer_ = 0.0f;
		deathStartScale_ = transform.scale;
		hasSpawnedExplosion_ = false;

		// トドメの一撃：強めの完全フリーズで「決まった」手応えを出す。
		// （撃破後はシーン側で撃破スローモーションへ繋がるが、
		//   それとは別レイヤーなので一瞬の“止め”を重ねても干渉しない）
		TimeManager::GetInstance()->RequestHitStop(HitStopPreset::Heavy());
		return;
	}

	SetAnimationIfChanged(animation_.HitReact);
	hitReactTimer_ = kHitReactDuration_;

	// 通常ヒット：当たった瞬間に軽くフリーズさせて打撃感を出す。
	// アニメーションだけでは伝わりづらかった「当てている実感」をここで補う。
	TimeManager::GetInstance()->RequestHitStop(HitStopPreset::Light());

	// 被弾リアクション（仰け反り）を起動：ヒットストップ中の震え＋つぶれ＋小さな後退。
	// 吹っ飛ばさず、プレイヤーと反対方向へ怯ませる。
	{
		Vector3 away{ 0.0f, 0.0f, 0.0f };
		if (target_) {
			away = transform.translate - target_->translate;
			away.y = 0.0f;
		}
		// プレイヤー情報が無い／真上で重なる等で向きが定まらないときは、自分の正面と逆へ。
		const float awayLen = std::sqrt(away.x * away.x + away.z * away.z);
		if (awayLen > 1e-4f) {
			hitFxRecoilDir_ = { away.x / awayLen, 0.0f, away.z / awayLen };
		}
		else {
			hitFxRecoilDir_ = { -std::sin(transform.rotate.y), 0.0f, -std::cos(transform.rotate.y) };
		}
		hitFxTimer_ = kHitFxDuration_;
	}

	// 被弾時のヒットパーティクルを発生
	if (deathSystem_) {
		Transform hitParticleTransform = transform;
		hitParticleTransform.translate.y += 3.5f; // 胴体あたりの高さ
		deathSystem_->EmitSystemByName("ADE", hitParticleTransform);
	}
}

void Enemy::CollectAliveTargets(std::vector<ITarget*>& out)
{
	// 自分（ボス）が生きていれば対象に加える。
	if (!isDead_) out.push_back(this);

	// 配下の雑魚のうち生存しているものを加える。
	for (auto& m : minions_) {
		if (m && m->IsAlive()) out.push_back(m.get());
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

void Enemy::StartDropIn()
{
    // 定位置の真上へ移してから落とす（着地点＝homePosition_ でブレない）
    transform.translate = homePosition_;
    transform.translate.y = homePosition_.y + kDropInHeight_;
    dropVelocity_ = 0.0f;
    dropping_ = true;
}

void Enemy::UpdateDropIn(float dt)
{
    // 自由落下（だんだん速くなるので「落ちてきた」感が出る）
    dropVelocity_ += kDropInGravity_ * dt;
    transform.translate.y -= dropVelocity_ * dt;

    if (transform.translate.y <= homePosition_.y) {
        transform.translate.y = homePosition_.y;
        dropVelocity_ = 0.0f;
        dropping_ = false;
        OnDropInLanded();
    }
}

void Enemy::OnDropInLanded()
{
    // 足元で土煙。着地の重さを見せる
    if (deathSystem_) {
        Transform burst = transform;
        burst.translate.y += kDropLandEffectOffsetY_;
        deathSystem_->EmitPreset(kDropLandPreset_, burst);
    }

    // 画面を揺らす
    if (cameraEffect_) {
        cameraEffect_->StartSimpleShake(kDropLandShakeDuration_, kDropLandShakeAmplitude_);
    }

    // 一瞬止めて衝撃を強調する
    TimeManager::GetInstance()->RequestHitStop(HitStopPreset::Heavy());
}

void Enemy::UpdateVisual()
{
	// 休眠中は見た目も更新しない（object3d_->Update のバリア発行を避ける）
	if (!active_) return;

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
	m->InitializeMinion(spawnPos);
	m->SetCamera(GetCamera());
	m->SetDamagePopupSink(damageSink_); // 雑魚にもダメージ表示の注入口を渡す
	minions_.push_back(std::move(m));
}

// ダメージ数値ポップアップの注入口をセット（自分＋既存の配下minionへ反映）
void Enemy::SetDamagePopupSink(IDamagePopupSink* sink)
{
	damageSink_ = sink;
	for (auto& m : minions_) {
		if (m) m->SetDamagePopupSink(sink);
	}
}

// 回転薙ぎ払いの着地エフェクト（衝撃波）を発生させる
void Enemy::SpawnSpinLandEffect(const Vector3& pos)
{
	if (!deathSystem_) return;

	// 敵と同じスケール感で衝撃波を出す（ADEエフェクトと同じ流儀）
	Transform t = transform;
	t.translate = pos;
	t.translate.y += 0.3f; // 地面に少し埋まらないよう持ち上げる
	t.rotate = { 0.0f, 0.0f, 0.0f };

	// 回転薙ぎ払い専用の衝撃波プリセット（Resources/Particle/SpinShockwave.json）
	deathSystem_->EmitByPresetName("SpinShockwave", t);
}

// 範囲攻撃判定を展開する（回転薙ぎ払い・ジャンプ急降下などから呼ばれる）
void Enemy::ActivateAreaAttack(float radius)
{
	spinHitboxRadius_ = radius;
	spinHitboxActive_ = true;
	UpdateSpinHitbox();
}

// 範囲攻撃判定を閉じる
void Enemy::DeactivateAreaAttack()
{
	spinHitboxActive_ = false;
	UpdateSpinHitbox();
}

// 判定が出ている間だけ非nullを返す
MultiCollider* Enemy::GetActiveAreaAttackCollider() const
{
	return spinHitboxActive_ ? spinHitbox_.get() : nullptr;
}

// 攻撃判定（球）をボス本体に追従させる
void Enemy::UpdateSpinHitbox()
{
	if (!spinHitbox_) return;
	Sphere& sp = spinHitbox_->MutableSphere(0);
	sp.center = transform.translate + colliderOffset_; // 胴体中心あたり
	sp.radius = spinHitboxActive_ ? spinHitboxRadius_ : 0.0f;
}

// 雑魚敵を1体ずつ順番に突進させるコーディネーター
//  ・出現演出が終わった雑魚敵をランダムに1体選び、突進～急降下させる
//  ・その1体が終わるまで他は待機（完全静止）
//  ・全員が突進し終えたら次のラウンドへ（再び1体ずつ繰り返す）
void Enemy::UpdateMinionCoordinator()
{
	if (minions_.empty()) {
		activeMinion_ = nullptr;
		return;
	}

	// activeMinion_ がまだリストに存在するか確認（死亡時の保険）
	if (activeMinion_) {
		bool alive = false;
		for (auto& m : minions_) {
			if (m.get() == activeMinion_) { alive = true; break; }
		}
		if (!alive) activeMinion_ = nullptr;
	}

	// 突進中の雑魚敵がいれば、それが終わるまで待つ
	if (activeMinion_) {
		if (activeMinion_->IsAttacking()) {
			return;
		}
		// 突進シーケンス完了 → このラウンドの行動済みにする
		activeMinion_->SetActed(true);
		activeMinion_ = nullptr;
	}

	// 次に突進させる候補（出現演出済み・未行動・待機中）を集める
	std::vector<MinionEnemy*> candidates;
	bool anySpawning = false;
	for (auto& m : minions_) {
		if (!m->IsSpawnFinished()) {
			anySpawning = true; // まだ出現演出中の雑魚敵がいる
			continue;
		}
		if (!m->HasActed() && m->IsIdle()) {
			candidates.push_back(m.get());
		}
	}

	if (!candidates.empty()) {
		// ランダムに1体選び、現在のプレイヤー位置へ突進させる
		static std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
		MinionEnemy* next = candidates[dist(rng)];

		Vector3 targetPos = next->GetTransform().translate;
		if (target_) {
			targetPos = target_->translate;
		}

		next->BeginAttack(targetPos);
		activeMinion_ = next;
		return;
	}

	// 候補なし：出現演出中の雑魚敵が残っているなら、その完了を待つ
	if (anySpawning) {
		return;
	}

	// 全員が突進し終えた → 次のラウンドへ（行動済みフラグをリセット）
	for (auto& m : minions_) {
		m->ResetRound();
	}
}
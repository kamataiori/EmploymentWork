#include "Enemy.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "EnemyDropBullet.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>
#include "engine/UI/UIManager.h"
#include <engine/UI/UIHpBar.h>
#include <EnemySplitBullet.h>

// Yaw(=Y回転)から OBB の3軸を作る簡易ヘルパ
static void BuildYawAxes(float yaw, Vector3 outAxes[3]) {
	const float c = std::cos(yaw);
	const float s = std::sin(yaw);
	// 右(X), 上(Y), 前(Z)
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
	obb.size = obbSize_;  // 半径

	Shape first{};
	first.kind = ShapeKind::OBB;
	first.obb = obb;

	*multiCollider_ = MultiCollider(first);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	// ヒットを Enemy::OnCollision に橋渡し
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });

	// === HPバー初期化 ===

	uiManager_ = std::make_unique<UIManager>();
	// 画面上中央
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
	// 死亡エフェクト用パーティクル（プリセット "fire" を使用）
	// -------------------------
	deathSystem_ = std::make_unique<ParticleManager>();
	deathSystem_->Initialize(VertexDataType::Plane);

	// プリセットと System を両方読み込む
	deathSystem_->LoadAllPresets();
	deathSystem_->LoadAllSystems();

	// Emit 時に使う Transform の初期値
	deathParticleTransform_ = transform;

}

void Enemy::Update()
{
	// ====== Δt（スローモーション対応） ======
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	if (!isDead_) {

		// -------------------------
		// AI（BT）で制御
		// -------------------------
		if (!isDead_) {
			if (aiController_) {
				aiController_->Update(dt);
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

			// ★ 分裂弾の順次発射制御
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

	// スケールを当たりにも反映したい場合はここで掛ける
	obb.size = { obbSize_.x,
				 obbSize_.y,
				 obbSize_.z };


	// HitReact の終了管理：一定時間で Idle に戻す
	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= dt;
		if (hitReactTimer_ <= 0.0f) {
			SetAnimationIfChanged(animation_.Idle);
			hitReactTimer_ = 0.0f;
		}
	}

	// ------------------------
	// オブジェクト更新処理
	// ------------------------
	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

#ifdef USE_IMGUI

	ImGui::Begin("Enemy");
	ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
	ImGui::End();
#endif // USE_IMGUI


	// ======== 死亡演出（縮小＋爆破＋タイトル遷移） ========
	if (isDead_) {
		// 経過時間
		deathTimer_ += dt;

		// Death アニメーションを少し見せてから縮小開始
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

		// マイナススケールに落ち込まないようにクランプ
		if (transform.scale.x < 0.0f) transform.scale.x = 0.0f;
		if (transform.scale.y < 0.0f) transform.scale.y = 0.0f;
		if (transform.scale.z < 0.0f) transform.scale.z = 0.0f;

		// 毎フレームパーティクルを更新
		if (deathSystem_) {
			deathSystem_->Update();
		}

		// 一定時間経ったら TITLE へ戻る
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

//==========================
// ★ 分裂弾の順次発射制御
//==========================
void Enemy::UpdateSplitBulletFiring(float dt)
{
	// 分裂弾がなければ何もしない
	if (splitBullets_.empty()) {
		splitFireActive_ = false;
		return;
	}

	// まだ順次発射モードに入っていない場合：
	// 全弾が SplitWait（分裂完了・発射待ち）になったかチェック
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
			// 全弾が分裂完了 → 順次発射モード開始
			splitFireActive_ = true;
			splitFireTimer_ = 0.0f;

			// ★ 最初の1発は即座に発射
			for (auto& b : splitBullets_) {
				if (b->IsReadyToShot()) {
					b->Fire();
					break;
				}
			}
		}
		return;
	}

	// 順次発射モード中：タイマーで1発ずつ発射
	splitFireTimer_ += dt;
	if (splitFireTimer_ >= splitFireInterval_)
	{
		splitFireTimer_ = 0.0f;

		// まだ待機中の弾を1つだけ発射
		for (auto& b : splitBullets_) {
			if (b->IsReadyToShot()) {
				b->Fire();
				break;  // 1発だけ
			}
		}

		// 全弾発射済みかチェック
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
	// コライダーの描画
	multiCollider_->Draw();

	// 落下弾（デバッグ球）
	for (auto& b : dropBullets_) {
		b->Draw();
	}

	for (auto& b : splitBullets_) {
		b->Draw();
	}
}

void Enemy::ForeGroundDraw()
{
	// === HPバー描画 ===
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

	// 弾パーティクル
	for (auto& b : splitBullets_) {
		b->ParticleDraw();
	}
}

void Enemy::OnCollision()
{
	// ===== HP減少 =====
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	// ===== HPチェック =====
	if (hp_ <= 0 && !isDead_) {
		// 死亡アニメーション（1回再生）
		object3d_->SetAnimationOneShot(animation_.Death);
		hitReactTimer_ = 0.0f;

		// 死亡ステートに入る
		isDead_ = true;
		deathTimer_ = 0.0f;

		// 縮小開始時のスケールを保存しておく
		deathStartScale_ = transform.scale;
		// 爆破はまだ
		hasSpawnedExplosion_ = false;

		return;
	}

	// ===== 被弾時アニメーション =====
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
	// 敵の少し上からスタート（4発同じ位置から上昇）
	Vector3 start = transform.translate;
	start.y += 2.0f;

	// パラメータ（調整用）
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

	// ★ 発射制御をリセット
	splitFireActive_ = false;
	splitFireTimer_ = 0.0f;
}
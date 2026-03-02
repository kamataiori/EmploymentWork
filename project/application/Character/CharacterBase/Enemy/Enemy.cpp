#include "Enemy.h"
#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>
#include "engine/UI/UIManager.h"
#include <engine/UI/UIHpBar.h>

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
		}

	}

	// 当たり判定中心を更新
	//colliderTranslate_ = transform.translate + colliderOffset_;
	colliderCenter_ = transform.translate + colliderOffset_;

	// コライダー更新
	/*Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;*/

	OBB& obb = multiCollider_->MutableOBB(0);
	obb.center = colliderCenter_;

	Vector3 axes[3];
	BuildYawAxes(transform.rotate.y, axes);
	obb.orientations[0] = axes[0];
	obb.orientations[1] = axes[1];
	obb.orientations[2] = axes[2];

	// スケールを当たりにも反映したい場合はここで掛ける
	obb.size = { obbSize_.x /** transform.scale.x*/,
				 obbSize_.y /** transform.scale.y*/,
				 obbSize_.z /** transform.scale.z*/ };


	// HitReact の終了管理：一定時間で Idle に戻す
	if (hitReactTimer_ > 0.0f) {
		hitReactTimer_ -= dt; // 可変ならΔtを使う
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
		// 経過時間（固定 60fps 前提）
		deathTimer_ += dt;

		// Death アニメーションを少し見せてから縮小開始
		const float kShrinkDelay = 0.8f;  // これだけ待ってから縮小
		const float kShrinkDuration = 1.0f;  // 縮小しきるまでの時間

		if (deathTimer_ >= kShrinkDelay) {
			// 0.0 ～ 1.0 の縮小進行度
			float t = (deathTimer_ - kShrinkDelay) / kShrinkDuration;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;

			// deathStartScale_ → 0 へ線形に縮小
			transform.scale.x = deathStartScale_.x * (1.0f - t);
			transform.scale.y = deathStartScale_.y * (1.0f - t);
			transform.scale.z = deathStartScale_.z * (1.0f - t);

			// スケールが 0 になったタイミングで一度だけ爆破
			if (!hasSpawnedExplosion_ && t >= 1.0f) {

				// 爆発の中心（少し上にオフセットして胸あたり）
				explosionPos = transform.translate;
				//explosionPos.y += 1.0f;

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

		// 毎フレームパーティクルを更新（Emitter は今のまま Update() 引数なし）
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

void Enemy::BackGroundDraw()
{
}

void Enemy::Draw()
{
	// コライダーの描画
	multiCollider_->Draw();

	
}

void Enemy::ForeGroundDraw()
{
	//OutputDebugStringA("Enemy::ForeGroundDraw called\n");

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
	// 相手のタイプIDを enum に戻す
	const auto otherType = static_cast<CollisionTypeIdDef>(info.otherType);

	// 要件：kPlayerWeapon と Enemy が当たった時にHP減少
	// Enemy側なので「相手が kPlayerWeapon」ならダメージを入れる
	if (otherType == CollisionTypeIdDef::kPlayerWeapon)
	{
		OnCollision();  // 既存のHP減少・死亡・被弾アニメ処理を使う
		return;
	}
}

void Enemy::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	deathSystem_->SetCamera(camera);


	// 必要なら武器や他のオブジェクトにもここで渡せる
	// if (weapon_) { weapon_->SetCamera(camera); } みたいな感じで拡張可能
}

void Enemy::SetAnimationIfChanged(const std::string& name)
{
	if (currentAnimationName_ != name) {
		object3d_->SetAnimation(name);
		currentAnimationName_ = name;
	}
}

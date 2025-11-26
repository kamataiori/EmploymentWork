#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include <SceneManager.h>

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

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	/*ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");*/
	ModelManager::GetInstance()->LoadModel("matest.obj");
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");
	ModelManager::GetInstance()->LoadModel("Sam.gltf");

	object3d_->SetModel("Skeleton.gltf");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f,30.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 3.0f, 3.0f, 3.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->SetAnimation(animation_.Idle);

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

	// AI初期
	state_ = RushState::Idle;
	stateTimer_ = idleTime_;


	// === HPバー初期化 ===
	hpBarBG_ = std::make_unique<Sprite>();
	hpBarFill_ = std::make_unique<Sprite>();

	// テクスチャは 2x2 の "hp"
	hpBarBG_->Initialize("Resources/hp.png");
	hpBarFill_->Initialize("Resources/hp.png");

	// 背景は少し暗め
	hpBarBG_->SetColor({ 0.2f, 0.2f, 0.2f, 0.8f });
	// 本体は赤系
	hpBarFill_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 初期サイズと位置（中央上）を一旦設定
	const float winW = 1280.0f;
	const float centerX = winW * 0.5f;
	const float left = centerX - hpBarMaxWidth_ * 0.5f;

	hpBarBG_->SetSize({ hpBarMaxWidth_, hpBarHeight_ });
	hpBarBG_->SetPosition({ left, hpBarTop_ });

	// fill は後で Update で現在HPに合わせて幅を更新
	hpBarFill_->SetSize({ hpBarMaxWidth_, hpBarHeight_ });
	hpBarFill_->SetPosition({ left, hpBarTop_ });

	hpBarBG_->SetAnchorPoint({ 0.0f, 0.5f });
	hpBarFill_->SetAnchorPoint({ 0.0f, 0.5f });

	// -------------------------
	// 死亡エフェクト用パーティクル（プリセット "fire" を使用）
	// -------------------------
	deathParticle_ = std::make_unique<ParticleManager>();
	deathParticle_->Initialize(ParticleManager::VertexDataType::Plane);

	// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	deathParticle_->LoadAllPresets();

	smokeParticle_ = std::make_unique<ParticleManager>();
	smokeParticle_->Initialize(ParticleManager::VertexDataType::Plane);

	// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	smokeParticle_->LoadAllPresets();

	ex1Particle_ = std::make_unique<ParticleManager>();
	ex1Particle_->Initialize(ParticleManager::VertexDataType::Plane);

	// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	ex1Particle_->LoadAllPresets();

	poweder = std::make_unique<ParticleManager>();
	poweder->Initialize(ParticleManager::VertexDataType::Plane);

	// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	poweder->LoadAllPresets();

	// Emit 時に使う Transform の初期値
	deathParticleTransform_ = transform;
	/*deathParticleTransform_.scale = { 1.0f, 1.0f, 1.0f };
	deathParticleTransform_.rotate = { 0.0f, 0.0f, 0.0f };
	deathParticleTransform_.translate = { 0.0f, 0.0f, 0.0f };*/

}

void Enemy::Update()
{
	// ====== 簡易AI：Idle → Dash → Cooldown ループ ======
   // ターゲットがいれば計算
	Vector3 toTargetXZ{ 0,0,0 };
	float desiredYaw = transform.rotate.y;

	if (target_) {
		Vector3 to = target_->translate - transform.translate;
		to.y = 0.0f;
		if (Length(to) > 1e-6f) {
			toTargetXZ = Normalize(to);
			// forward = (sinYaw, 0, cosYaw) の想定
			desiredYaw = std::atan2(toTargetXZ.x, toTargetXZ.z);
		}
	}

	if (!isDead_) {

		switch (state_) {
		case RushState::Idle:
			// 常にプレイヤーへ向く（スムーズ）
			transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_);
			stateTimer_ -= 1.0f / 60.0f;
			if (stateTimer_ <= 0.0f) {
				// ダッシュへ移行：向いている方向を固定
				if (target_) {
					dashDir_ = toTargetXZ;                 // 方向確定（XZ）
					transform.rotate.y = desiredYaw;       // 顔をピタッと正面へ
				}
				else {
					// ターゲットが無い場合は今の向きで前進
					dashDir_ = { std::sin(transform.rotate.y), 0.0f, std::cos(transform.rotate.y) };
				}
				state_ = RushState::Dash;
				stateTimer_ = dashTime_;
				SetAnimationIfChanged(animation_.Run);
			}
			break;

		case RushState::Dash:
			// 突進（※ 突進中はプレイヤーへ向き直ししない）
			transform.translate.x += dashDir_.x * dashSpeed_;
			transform.translate.z += dashDir_.z * dashSpeed_;

			stateTimer_ -= 1.0f / 60.0f;
			if (stateTimer_ <= 0.0f) {
				state_ = RushState::Cooldown;
				stateTimer_ = cooldownTime_;
				SetAnimationIfChanged(animation_.Idle);
			}
			break;

		case RushState::Cooldown:
			// 向きだけは緩やかにターゲットへ（次のダッシュ準備）
			transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_ * 0.6f);
			stateTimer_ -= 1.0f / 60.0f;
			if (stateTimer_ <= 0.0f) {
				state_ = RushState::Idle;
				stateTimer_ = idleTime_;
				SetAnimationIfChanged(animation_.Idle);
			}
			break;
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
		hitReactTimer_ -= 1.0f / 60.0f; // 固定60FPS前提。可変ならΔtを使う
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
		deathTimer_ += 1.0f / 60.0f;

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

				 // fire プリセットをこの位置で Emit
				if (deathParticle_) {
					deathParticleTransform_.translate = explosionPos;
					deathParticleTransform_.translate.y += 3.5f;
					deathParticle_->EmitByPresetName("fire", deathParticleTransform_);
				}
				if (smokeParticle_) {
					deathParticleTransform_.translate = explosionPos;
					deathParticleTransform_.translate.y += 2.5f;
					smokeParticle_->EmitByPresetName("smoke", deathParticleTransform_);
				}
				if (ex1Particle_) {
					deathParticleTransform_.translate = explosionPos;
					deathParticleTransform_.translate.y += 3.5f;
					ex1Particle_->EmitByPresetName("ex1", deathParticleTransform_);
				}
				if (poweder) {
					deathParticleTransform_.translate = explosionPos;
					deathParticleTransform_.translate.y += 2.5f;
					poweder->EmitByPresetName("powder", deathParticleTransform_);
				}

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
		if (deathParticle_) {
			deathParticle_->Update();
		}

		if (smokeParticle_) {
			smokeParticle_->Update();
		}

		if (ex1Particle_) {
			ex1Particle_->Update();
		}

		if (poweder) {
			poweder->Update();
		}


		// 一定時間経ったら TITLE へ戻る
		if (deathTimer_ >= kDeathToTitleDelay_) {
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
		return;
	}


	const float ratio = (kMaxHP_ > 0) ? std::clamp(hp_ / float(kMaxHP_), 0.0f, 1.0f) : 0.0f;

	const float winW = 1280.0f;
	const float centerX = winW * 0.5f;
	const float maxW = hpBarMaxWidth_;
	const float curW = maxW * ratio;
	const float leftBG = centerX - maxW * 0.5f;
	const float leftFill = leftBG;

	// 背景は常に最大幅
	hpBarBG_->SetSize({ maxW, hpBarHeight_ });
	hpBarBG_->SetPosition({ leftBG, hpBarTop_ });
	hpBarBG_->Update();

	// 本体は現在幅
	hpBarFill_->SetSize({ curW, hpBarHeight_ });
	hpBarFill_->SetPosition({ leftFill, hpBarTop_ });
	hpBarFill_->Update();
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
	// === HPバー描画 ===
	if (hpBarBG_ && hpBarFill_) {

		// 背景 → 本体の順で描画
		hpBarBG_->Draw();
		hpBarFill_->Draw();
	}
}

void Enemy::AnimationDraw()
{
	object3d_->Draw();
}

void Enemy::ParticleDraw()
{
	if (isDead_ && deathParticle_) {
		deathParticle_->Draw();
	}

	if (isDead_ && smokeParticle_) {
		smokeParticle_->Draw();
	}

	if (isDead_ && ex1Particle_) {
		ex1Particle_->Draw();
	}

	if (isDead_ && poweder) {
		poweder->Draw();
	}
}

void Enemy::OnCollision()
{
	// ===== HP減少 =====
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	/*if (poweder) {
		deathParticleTransform_.translate = explosionPos;
		deathParticleTransform_.translate.y += 2.5f;
		poweder->EmitByPresetName("powder", deathParticleTransform_);
	}*/

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

void Enemy::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	deathParticle_->SetCamera(camera);
	smokeParticle_->SetCamera(camera);
	ex1Particle_->SetCamera(camera);
	poweder->SetCamera(camera);


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

#include "PlayerBase.h"
#include "PlayerWeaponOBB.h"
#include <FollowCamera.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

static float WrapPi(float a) {
	while (a > std::numbers::pi_v<float>) a -= 2.0f * std::numbers::pi_v<float>;
	while (a < -std::numbers::pi_v<float>) a += 2.0f * std::numbers::pi_v<float>;
	return a;
}

void PlayerBase::Initialize()
{
	// object3dの初期化
	object3d_->Initialize();

	const char* modelName = GetModelName();
	ModelManager::GetInstance()->LoadModel(modelName);
	object3d_->SetModel(modelName);

	// ここを毎回実行しない
	if (isFirstInitialize_) {
		//const float halfH = 1.0f; // = playerOBB.size.y と同じ値
		transform.translate = { 0.0f, 0.0f, -10.0f }; // 底がちょうどy=0に来る
		transform.rotate = { 0.0f, 0.0f,  0.0f };
		transform.scale = { 1.0f, 1.0f,  1.0f };
		isFirstInitialize_ = false;
	}

	// ここは常に現在のtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	weapon_ = std::make_unique<PlayerWeaponOBB>();
	weapon_->SetOwner(this);
	weapon_->SetPlayerTransform(&transform);
	weapon_->Initialize();

	// コライダーを生成
	// --- 原点が足元 → 股下(例: 上方向0.9f) にオフセット ---
	colliderOffset_ = { 0.0f, 1.0f, 0.0f };
	colliderTranslate_ = transform.translate + colliderOffset_;

	Sphere playerSp{};
	playerSp.center = colliderTranslate_;
	playerSp.radius = sphereRadius_;

	Shape first{};
	first.kind = ShapeKind::Sphere;
	first.sphere = playerSp;

	*multiCollider_ = MultiCollider(first);
	// 種別登録
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));
	// コールバック登録
	multiCollider_->SetHitCallback([this]() { this->OnCollision(); });


	particle->Initialize(ParticleManager::VertexDataType::Plane);
	particle->CreateParticleGroup("particle", "Resources/circle.png", ParticleManager::BlendMode::kBlendModeAdd);
	auto emitter = std::make_unique<ParticleEmitter>();
	emitter->Initialize(
		particle.get(),
		"particle",
		Transform{ {1.0f, 1.0f, 0.0f}, {0.0f,0.0f,0.0f}, {1.0f,4.0f,1.0f} },
		EmitterConfig{ ShapeType::Plane, 100, 0.5f, true }
	);
	emitters.push_back(std::move(emitter));
}

void PlayerBase::Update()
{
	// ロックの減衰
	if (animLockTimer_ > 0.0f) {
		animLockTimer_ -= 1.0f / 60.0f;
		if (animLockTimer_ <= 0.0f) {
			animLockTimer_ = 0.0f;
			currentAnimPriority_ = 0;
			// 止まっている場合に備えて一度Idleを要求しておく
			RequestAnimKey(PlayerAnimKey::Idle, 0);
		}
	}

	// playerの基本となる動きの呼出し
	Move();

	if (weapon_) {
		weapon_->Update();
		weapon_->NormalAttack();
		weapon_->Skill();
		weapon_->Ultimate();
	}

	//ImGui::Begin("player");
	//ImGui::DragFloat3("translate", &transform.translate.x);
	//ImGui::DragFloat3("Collider Offset", &colliderOffset_.x, 0.01f);
	//ImGui::DragFloat("Sphere Radius", &sphereRadius_, 0.01f, 0.0f, 10.0f);

	//// 当たり判定の可視化
	//if (isCollided_) {
	//	ImGui::TextColored(ImVec4(1, 0, 0, 1), "Hit! (Collision Detected)");
	//}
	//else {
	//	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Collision");
	//}
	//ImGui::End();

	// --- 当たり判定の中心を更新 ---
	colliderTranslate_ = transform.translate + colliderOffset_;

	// ------------------------
	// オブジェクト更新処理
	// ------------------------
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// コライダー位置を更新
	// OBB をプレイヤーTransformに追従させる
	// 今回は最初の形状(0)を更新する想定
	isCollided_ = false;
	Sphere& sp = multiCollider_->MutableSphere(0); // MultiCollider 側に MutableSphere(index) がある前提
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;



	for (auto& emitter : emitters)
	{
		emitter->Update();
	}
	particle->Update();
}

void PlayerBase::BackGroundDraw()
{
}

void PlayerBase::Draw()
{
	multiCollider_->Draw();
	weapon_->Draw();
}

void PlayerBase::ForeGroundDraw()
{
}

void PlayerBase::AnimationDraw()
{
	object3d_->Draw();
}

void PlayerBase::ParticleDraw()
{
	particle->Draw();
}

void PlayerBase::OnCollision()
{
	// ====== HP減少処理 ======
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	// ====== HPチェック ======
	if (hp_ <= 0) {
		// 死亡アニメーション
		//PlayAnimKey(PlayerAnimKey::Death);

		object3d_->SetAnimationOneShot("Death");

		//// デバッグ出力
		//ImGui::Begin("Player HP");
		//ImGui::TextColored(ImVec4(1, 0, 0, 1), "Player Died! HP = 0");
		//ImGui::End();

		return; // 死亡時はここで抜けて以降の処理を止める
	}

	// ====== 被弾時アニメーション（生存時のみ） ======
	PlayAnimKey(PlayerAnimKey::Hit2);

	// 当たった時にフラグON
	isCollided_ = true;
}

void PlayerBase::Move()
{
	// -------------------------------
	// 入力による左右・前後移動処理
	// -------------------------------
	// -------------------------------
	// WASD入力による方向ベクトル計算
	// -------------------------------
	
	// 1) カメラのヨー角（FollowCamera想定）
	float camYaw = 0.0f;
	if (auto fc = dynamic_cast<FollowCamera*>(camera_)) {
		camYaw = fc->GetAngle(); // カメラの“後ろ向き”角
	}
	// カメラ正面（プレイヤーが向くべき前）は angle + π
	const float playerFaceYaw = camYaw + std::numbers::pi_v<float>;

	// 2) カメラ基底ベクトル（XZ）
	const Vector3 cameraForwardXZ = { -std::sin(camYaw), 0.0f, -std::cos(camYaw) };
	const Vector3 cameraRightXZ = { -std::cos(camYaw), 0.0f, std::sin(camYaw) };

	// 3) WASDをカメラ相対で合成（D=+Right / A=-Right）
	Vector3 wishDir = { 0,0,0 };
	if (Input::GetInstance()->PushKey(DIK_W)) wishDir += cameraForwardXZ;
	if (Input::GetInstance()->PushKey(DIK_S)) wishDir -= cameraForwardXZ;
	if (Input::GetInstance()->PushKey(DIK_D)) wishDir += cameraRightXZ;
	if (Input::GetInstance()->PushKey(DIK_A)) wishDir -= cameraRightXZ;

	bool isMoving = (Length(wishDir) > 0.0001f);
	if (isMoving) wishDir = Normalize(wishDir);

	// 4) 移動
	const float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;
	if (isMoving) {
		transform.translate.x += wishDir.x * currentSpeed;
		transform.translate.z += wishDir.z * currentSpeed;
	}

	// 5) 目標ヨー角：移動中は移動方向、停止中はカメラ正面
	const float targetYaw = isMoving
		? std::atan2(wishDir.x, wishDir.z)
		: playerFaceYaw;

	// 6) スムーズ回転（最短角＆角速度クランプ）
	const float curYaw = transform.rotate.y;
	float deltaYaw = WrapPi(targetYaw - curYaw);
	const float turnSpeed = 2.5f;
	const float turnRate = std::numbers::pi_v<float> * turnSpeed; // 180deg/s（好みで調整可）
	const float dt = 1.0f / 60.0f;              // 可変フレームなら実Δtを使う
	const float maxStep = turnRate * dt;

	if (deltaYaw > maxStep) deltaYaw = maxStep;
	if (deltaYaw < -maxStep) deltaYaw = -maxStep;
	transform.rotate.y = curYaw + deltaYaw;

	// 7) アニメ（ロック中は移動アニメ出さない）
	if (animCtrl_) {
		if (!IsAnimLocked()) {
			if (isMoving)  RequestAnimKey(PlayerAnimKey::RunWeapon, 0);
			else           RequestAnimKey(PlayerAnimKey::Idle, 0);
		}
	}

	// -------------------------------
	// ジャンプ処理呼出し
	// -------------------------------

	Jump();

	// -------------------------------
	// ブリンク(ダッシュ)処理呼出し
	// -------------------------------

	Blink();


}

void PlayerBase::Jump()
{
	// ----------------
	// 二段ジャンプ処理
	// ----------------
	// -------------------------------
	// 有効半径（スケール対応：最大軸で拡大）
	const float effectiveRadius =
		sphereRadius_ * std::max({ transform.scale.x, transform.scale.y, transform.scale.z });

	// 現在の当たり中心Y（足元原点 + オフセット）
	const float colliderCenterY = transform.translate.y + colliderOffset_.y;

	// 現在の足底Y
	const float bottomNow = colliderCenterY - effectiveRadius;

	// 1 接地クランプ（非ジャンプ時の保険）
	if (!jump_.isJumping && bottomNow < jump_.kGroundHeight) {
		const float targetCenterY = jump_.kGroundHeight + effectiveRadius;    // 当たり中心Y
		transform.translate.y = targetCenterY - colliderOffset_.y;            // モデル原点Yに戻す
		// リセット
		jump_.isJumping = false;
		jump_.velocity = 0.0f;
		jump_.jumpCount = 0;
		jump_.canJump_ = true;
		move_.hasDashed_ = false;
	}

	// 2 ジャンプ入力（2段まで）
	if (jump_.canJump_ && Input::GetInstance()->PushKey(DIK_SPACE) &&
		jump_.jumpCount < jump_.kMaxJumpCount) {

		jump_.velocity = jump_.kInitialVelocity;
		jump_.isJumping = true;
		jump_.jumpCount++;
		jump_.canJump_ = false; // 離すまで再ジャンプ禁止
	}
	if (!Input::GetInstance()->PushKey(DIK_SPACE)) {
		jump_.canJump_ = true;
	}

	// 3 速度反映（予測→着地判定→クランプ）
	if (jump_.isJumping) {
		// 次フレームの原点Yを予測
		const float nextY = transform.translate.y + jump_.velocity;

		// 次フレームの当たり中心と足底
		const float nextCenterY = nextY + colliderOffset_.y;
		const float bottomNext = nextCenterY - effectiveRadius;

		if (jump_.velocity <= 0.0f && bottomNext < jump_.kGroundHeight) {
			// 下向き移動中に地面をまたぐ → ちょうど着地位置へクランプ
			const float targetCenterY = jump_.kGroundHeight + effectiveRadius;
			transform.translate.y = targetCenterY - colliderOffset_.y;

			// 着地リセット
			jump_.isJumping = false;
			jump_.velocity = 0.0f;
			jump_.jumpCount = 0;
			jump_.canJump_ = true;
			move_.hasDashed_ = false;
		}
		else {
			// まだ空中：位置更新＆重力
			transform.translate.y = nextY;
			jump_.velocity -= jump_.kGravity;
		}
	}
}

void PlayerBase::Blink()
{
	// -------------------------------
	// ダッシュ制御：1回だけ発動可能
	// -------------------------------
	// クールダウンを減算
	if (move_.dashCooldown > 0.0f) {
		move_.dashCooldown -= 1.0f / 60.0f;
		if (move_.dashCooldown < 0.0f) move_.dashCooldown = 0.0f;
	}

	// 押下・解放状態
	const bool dashHeld = Input::GetInstance()->PushMouseButton(1);

	// 接地判定
	const float groundEps = 0.001f;
	const bool isGrounded = (!jump_.isJumping) && (transform.translate.y <= jump_.kGroundHeight + groundEps);

	// ---- 起動条件：押した・ダッシュ中でない・クールダウン終わり ----
	if (!move_.isDashing && dashHeld && !move_.isDashKeyHeld_ && move_.dashCooldown <= 0.0f) {
		move_.isDashing = true;
		move_.dashTimer = move_.kDashDuration;
		move_.hasDashed_ = true;
		move_.isDashKeyHeld_ = true;

		const float yaw = transform.rotate.y;
		move_.dashDir = Normalize(Vector3{ std::sin(yaw), 0.0f, std::cos(yaw) });

		PostEffectManager::GetInstance()->SetType(PostEffectType::RadialBlur);
	}

	// 右クリック解放を検出（次の押下のためのエッジ作り）
	if (!dashHeld) {
		move_.isDashKeyHeld_ = false;
	}

	// ---- ダッシュ中の処理 ----
	if (move_.isDashing) {
		move_.dashTimer -= 1.0f / 60.0f;
		transform.translate.x += move_.dashDir.x * move_.dashSpeed;
		transform.translate.z += move_.dashDir.z * move_.dashSpeed;

		if (move_.dashTimer <= 0.0f) {
			move_.isDashing = false;
			move_.dashTimer = 0.0f;
			move_.dashCooldown = move_.kDashCooldown;      // 終了後にクールダウン開始
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
		}
	}
	else {
		// ---- 再装填条件：クールダウン終了 ＆ 右クリック離している ＆（できれば接地）----
		if (move_.dashCooldown <= 0.0f && !dashHeld && isGrounded) {
			move_.hasDashed_ = false;  // これで2回目以降も使える
		}
	}
}

void PlayerBase::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	if (particle) {
		particle->SetCamera(camera);
	}

	// 必要なら武器や他のオブジェクトにもここで渡せる
	// if (weapon_) { weapon_->SetCamera(camera); } みたいな感じで拡張可能
}

void PlayerBase::ChangeModel(const char* modelName)
{
	object3d_->SetModel(modelName);
}

void PlayerBase::PlayAnimKey(PlayerAnimKey key)
{
	if (!animCtrl_) return;
	SetAnimationIfChanged(animCtrl_->Resolve(key));
}

void PlayerBase::RequestAnimKey(PlayerAnimKey key, int priority, float lockSec)
{
	// 低い優先度からの上書きは禁止（攻撃中に移動で潰さない）
	if (priority < currentAnimPriority_) return;

	PlayAnimKey(key);
	currentAnimPriority_ = priority;

	// ロックは長い方を採用で上書き
	if (lockSec > 0.0f) {
		if (animLockTimer_ < lockSec) animLockTimer_ = lockSec;
	}
}

void PlayerBase::SetAnimationIfChanged(const std::string& name)
{
	if (name.empty()) return;              // 安全ガード
	if (currentAnimationName_ == name) return;
	object3d_->SetAnimation(name);
	currentAnimationName_ = name;
}

#include "Player.h"
#include "PlayerWeaponOBB.h"
#include <FollowCamera.h>
#include "engine/TimeManager.h"

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

void Player::Initialize()
{
	// object3dの初期化
	object3d_->Initialize();

	
	object3d_->SetModel("Warrior.gltf");

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

	sword_ = std::make_unique<Sword>(baseScene_);
	sword_->Initialize();
	sword_->SetCamera(camera_); // camera_ が後で入るなら SetCamera() 側でも呼ぶ

	// ボーン名は実際の名前に合わせて修正が必要
	sword_->AttachTo(object3d_.get(), "Fist.R");

	
	//poweder = std::make_unique<ParticleManager>();
	//poweder->Initialize(ParticleManager::VertexDataType::Plane);

	//// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	//poweder->LoadAllPresets();
}

void Player::Update()
{
	// ===== Δt（スロー対応） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// ロックの減衰
	if (animaLockTimer_ > 0.0f) {
		animaLockTimer_ -= dt;
		if (animaLockTimer_ <= 0.0f) {
			animaLockTimer_ = 0.0f;
			currentAnimaPriority_ = 0;
			// 止まっている場合に備えて一度Idleを要求しておく
			//RequestAnimaKey(PlayerAnimKey::IdleWeapon, 0);
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

	if (sword_) {
		sword_->Update();
	}

	bool weaponAttacking = false;
	if (auto w = dynamic_cast<PlayerWeaponOBB*>(weapon_.get())) {
		weaponAttacking = w->IsAttacking();
	}

	if (!IsAnimaLocked() && !weaponAttacking) {
		if (isMoving_)  RequestAnimaKey(PlayerAnimKey::RunWeapon, 0);
		else            RequestAnimaKey(PlayerAnimKey::Idle, 0);
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

	/*poweder->Update();*/
}

void Player::BackGroundDraw()
{
}

void Player::Draw()
{
	multiCollider_->Draw();
	weapon_->Draw();
	sword_->Draw();
}

void Player::ForeGroundDraw()
{
	
}

void Player::AnimationDraw()
{
	object3d_->Draw();
}

void Player::ParticleDraw()
{
	/*poweder->Draw();*/
}

void Player::OnCollision()
{
	// 相手が敵でなければ何もしない（武器や弾との衝突を無視）
	/*if (other->GetTypeID() != (uint32_t)CollisionTypeIdDef::kEnemy) {
		return;
	}*/

	// ====== HP減少処理 ======
	/*hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;*/

	/*poweder->EmitByPresetName("powder", transform);
	poweder->Update();*/

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
	//PlayAnimaKey(PlayerAnimKey::RecieveHit);

	// 当たった時にフラグON
	isCollided_ = true;
}

void Player::Move()
{
	// ===== Δt（回転のスムージング用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// 1) カメラのヨー角（FollowCamera想定）
	float camYaw = 0.0f;
	if (auto fc = dynamic_cast<FollowCamera*>(camera_)) {
		camYaw = fc->GetAngle(); // カメラの“後ろ向き”角
	}

	// カメラ正面（プレイヤーが向き続けるべき前）
	const float playerFaceYaw = camYaw + std::numbers::pi_v<float>;

	// 2) カメラ基底ベクトル（XZ）
	const Vector3 cameraForwardXZ = { -std::sin(camYaw), 0.0f, -std::cos(camYaw) };
	const Vector3 cameraRightXZ = { -std::cos(camYaw), 0.0f,  std::sin(camYaw) };

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

	// 5) 目標ヨー角：常に「正面固定」（移動方向では回さない！）
	const float targetYaw = playerFaceYaw;

	// 6) スムーズ回転（最短角＆角速度クランプ）
	const float curYaw = transform.rotate.y;
	float deltaYaw = WrapPi(targetYaw - curYaw);
	const float turnSpeed = 2.5f;
	const float turnRate = std::numbers::pi_v<float> *turnSpeed; // 180deg/s
	const float maxStep = turnRate * dt;

	if (deltaYaw > maxStep) deltaYaw = maxStep;
	if (deltaYaw < -maxStep) deltaYaw = -maxStep;
	transform.rotate.y = curYaw + deltaYaw;

	isMoving_ = isMoving;

	// 7) アニメ（Wで進む時と同じでOK）
	// if (!IsAnimaLocked()) {
	//     if (isMoving) RequestAnimaKey(PlayerAnimKey::RunWeapon, 0);
	//     else          RequestAnimaKey(PlayerAnimKey::Idle, 0);
	// }

	// ジャンプ / ブリンク
	Jump();
	Blink();
}


void Player::Jump()
{
	// 有効半径（スケール対応：最大軸で拡大）
	const float effectiveRadius =
		sphereRadius_ * std::max({ transform.scale.x, transform.scale.y, transform.scale.z });

	// 現在の当たり中心Y（足元原点 + オフセット）
	const float colliderCenterY = transform.translate.y + colliderOffset_.y;

	// 現在の足底Y
	const float bottomNow = colliderCenterY - effectiveRadius;

	// 接地判定（ほぼ地面にいるか）
	const bool isGrounded = (bottomNow <= jump_.kGroundHeight + 0.0001f);

	// 1 接地クランプ（非ジャンプ時の保険）
	if (!jump_.isJumping && bottomNow < jump_.kGroundHeight) {
		const float targetCenterY = jump_.kGroundHeight + effectiveRadius;
		transform.translate.y = targetCenterY - colliderOffset_.y;

		// 着地リセット
		jump_.isJumping = false;
		jump_.velocity = 0.0f;
		jump_.jumpCount = 0;
		jump_.canJump_ = true;
		move_.hasDashed_ = false;
	}

	// 2 ジャンプ入力（★地面にいる時だけ / 1段のみ）
	if (isGrounded && jump_.canJump_ && Input::GetInstance()->PushKey(DIK_SPACE)) {
		jump_.velocity = jump_.kInitialVelocity;
		jump_.isJumping = true;
		jump_.jumpCount = 1;      // 保持したいなら（なくてもOK）
		jump_.canJump_ = false;   // 離すまで再ジャンプ禁止
	}
	if (!Input::GetInstance()->PushKey(DIK_SPACE)) {
		jump_.canJump_ = true;
	}

	// 3 速度反映（予測→着地判定→クランプ）
	if (jump_.isJumping) {
		const float nextY = transform.translate.y + jump_.velocity;

		const float nextCenterY = nextY + colliderOffset_.y;
		const float bottomNext = nextCenterY - effectiveRadius;

		if (jump_.velocity <= 0.0f && bottomNext < jump_.kGroundHeight) {
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
			transform.translate.y = nextY;
			jump_.velocity -= jump_.kGravity;
		}
	}
}

void Player::Blink()
{
	// ===== Δt（クールダウン・ダッシュ時間用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// -------------------------------
	// ダッシュ制御：1回だけ発動可能
	// -------------------------------
	// クールダウンを減算
	if (move_.dashCooldown > 0.0f) {
		move_.dashCooldown -= dt;
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
		move_.dashTimer -= dt;
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

void Player::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	sword_->SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	/*if (poweder) {
		poweder->SetCamera(camera);
	}*/

	// 必要なら武器や他のオブジェクトにもここで渡せる
	// if (weapon_) { weapon_->SetCamera(camera); } みたいな感じで拡張可能
}

void Player::PlayAnimaKey(PlayerAnimKey key)
{
	SetAnimationIfChanged(animaCtrl_.Resolve(key));
}

void Player::RequestAnimaKey(PlayerAnimKey key, int priority, float lockSec)
{
	// 低い優先度からの上書きは禁止（攻撃中に移動で潰さない）
	if (priority < currentAnimaPriority_) return;

	PlayAnimaKey(key);
	currentAnimaPriority_ = priority;

	// ロックは長い方を採用で上書き
	if (lockSec > 0.0f) {
		if (animaLockTimer_ < lockSec) animaLockTimer_ = lockSec;
	}
}

void Player::SetAnimationIfChanged(const std::string& name)
{
	if (name.empty()) return;              // 安全ガード
	if (currentAnimationName_ == name) return;
	object3d_->SetAnimation(name);
	currentAnimationName_ = name;
}

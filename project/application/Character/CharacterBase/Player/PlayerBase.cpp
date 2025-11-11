#include "PlayerBase.h"
#include "PlayerWeaponOBB.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

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

	mc_ = std::make_unique<MultiCollider>(first);
	mc_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer)); // 種別は従来と同じ扱い
	SetCollider(mc_.get()); // ObjectBase 側に差す（従来の this ではなく mc_ を渡す）
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
	}

	ImGui::Begin("player");
	ImGui::DragFloat3("translate", &transform.translate.x);
	ImGui::DragFloat3("Collider Offset", &colliderOffset_.x, 0.01f);
	ImGui::DragFloat("Sphere Radius", &sphereRadius_, 0.01f, 0.0f, 10.0f);
	ImGui::End();

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
	Sphere& sp = mc_->MutableSphere(0); // MultiCollider 側に MutableSphere(index) がある前提
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;
}

void PlayerBase::Draw()
{
	// SphereCollider の描画
	//SphereCollider::Draw();

	mc_->Draw();
}

void PlayerBase::SkinningDraw()
{
	object3d_->Draw();
}

void PlayerBase::ParticleDraw()
{
}

//void PlayerBase::OnCollision()
//{
//	//sphere.color = static_cast<int>(Color::RED);
//}

void PlayerBase::Move()
{
	// -------------------------------
	// ダッシュ制御：1回だけ発動可能
	// -------------------------------
	// Bキーを初めて押した瞬間だけダッシュ許可（空中でも可）
	if (!move_.isDashKeyHeld_ && Input::GetInstance()->PushKey(DIK_B) && !move_.hasDashed_) {
		move_.isDashing = true;
		move_.dashTimer = move_.kDashDuration;
		move_.hasDashed_ = true;
		move_.isDashKeyHeld_ = true;
	}

	// Bキーを離したら、次の押下を受付可能にする
	if (!Input::GetInstance()->PushKey(DIK_B)) {
		move_.isDashKeyHeld_ = false;
	}

	// ダッシュ中タイマー処理
	if (move_.isDashing) {
		//SetAnimationIfChanged(animation_.Roll);
		move_.dashTimer -= 1.0f / 60.0f; // フレーム単位で減算（60FPS想定）
		if (move_.dashTimer <= 0.0f) {
			move_.isDashing = false;
			move_.dashTimer = 0.0f;
		}
		PostEffectManager::GetInstance()->SetType(PostEffectType::RadialBlur);
	}
	else {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
	}

	// -------------------------------
	// 入力による左右・前後移動処理
	// -------------------------------
	// -------------------------------
	// WASD入力による方向ベクトル計算
	// -------------------------------
	move_.direction = { 0.0f, 0.0f, 0.0f };

	bool isMoving = false;

	if (Input::GetInstance()->PushKey(DIK_W)) {
		move_.direction.z += 1.0f;
		isMoving = true;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		move_.direction.z -= 1.0f;
		isMoving = true;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		move_.direction.x -= 1.0f;
		isMoving = true;
	}
	if (Input::GetInstance()->PushKey(DIK_D)) {
		move_.direction.x += 1.0f;
		isMoving = true;
	}

	// 正規化してプレイヤーの向きに合わせた移動に変換
	if (Length(move_.direction) > 0.0f) {
		move_.direction = Normalize(move_.direction);
		float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;

		// Y軸の回転行列を生成（プレイヤーの向きに応じた回転）
		Matrix4x4 rotY = MakeRotateYMatrix(transform.rotate.y);

		// 入力方向ベクトルをプレイヤーの向きに回転
		Vector3 rotatedDir = TransformVector(move_.direction, rotY);

		// 回転後の方向に沿って移動
		transform.translate.x += rotatedDir.x * currentSpeed;
		transform.translate.z += rotatedDir.z * currentSpeed;
	}

	/*const auto& anim = GetAnimation();

	if (isMoving) {
		SetAnimationIfChanged(anim.Run_Weapon);
	}
	else {
		SetAnimationIfChanged(anim.Idle);
	}*/

	//if (animCtrl_) {
	//	if (!IsAnimLocked()) { // ★攻撃などでロック中は移動アニメを出さない
	//		if (isMoving) {
	//			RequestAnimKey(PlayerAnimKey::RunWeapon, 0);   // 優先度0
	//		}
	//		else {
	//			RequestAnimKey(PlayerAnimKey::Idle, 0);        // 優先度0
	//		}
	//	}
	//}

	if (animCtrl_) {
		if (!IsAnimLocked()) {
			if (isMoving)  RequestAnimKey(PlayerAnimKey::RunWeapon, 0);
			else           RequestAnimKey(PlayerAnimKey::Idle, 0);
		}
	}


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

void PlayerBase::ChangeModel(const char* modelName)
{
	//ModelManager::GetInstance()->LoadModel(modelName);
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

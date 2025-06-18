#include "Player.h"
#include <FollowCamera.h>

void Player::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");

	object3d_->SetModel("uvChecker.gltf");

	// 初期Transform設定
	transform.translate = { -2.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 3.14f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	sphere.radius = 2.0f;
	SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

}

void Player::Update()
{
	// 移動とジャンプ処理
	Move();

	FollowCamera* followCam = dynamic_cast<FollowCamera*>(camera_);
	if (followCam) {
		transform.rotate.y = followCam->GetAngle();
	}

	// 弾の処理
	HandleBullet();

	// -------------------------------
	// ImGui による Transform 調整
	// -------------------------------
	ImGui::Begin("Player Transform");

	if (ImGui::DragFloat3("Position", &transform.translate.x, 0.1f)) {}
	if (ImGui::DragFloat3("Rotation", &transform.rotate.x, 0.1f)) {}
	if (ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f, 0.1f, 10.0f)) {}

	ImGui::End();

	// ------------------------
	// オブジェクト更新処理
	// ------------------------
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// コライダー位置を更新
	SetPosition(transform.translate);

	sphere.color = static_cast<int>(Color::WHITE);
}

void Player::Draw()
{
	object3d_->Draw();
	// SphereCollider の描画
	SphereCollider::Draw();

	if (bullet_) {
		bullet_->Draw();
	}
}

void Player::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);
}

void Player::Move()
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

	if (Input::GetInstance()->PushKey(DIK_W)) {
		move_.direction.z += 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		move_.direction.z -= 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		move_.direction.x -= 1.0f;
	}
	if (Input::GetInstance()->PushKey(DIK_D)) {
		move_.direction.x += 1.0f;
	}

	//// 正規化して一定速度で移動
	//if (Length(move_.direction) > 0.0f) {
	//	move_.direction = Normalize(move_.direction);
	//	float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;
	//	transform.translate.x += move_.direction.x * currentSpeed;
	//	transform.translate.z += move_.direction.z * currentSpeed;
	//}

	// 正規化してプレイヤーの向きに合わせた移動に変換
	if (Length(move_.direction) > 0.0f) {
		move_.direction = Normalize(move_.direction);
		float currentSpeed = move_.isDashing ? move_.dashSpeed : move_.speed;

		// Y軸の回転行列を生成（プレイヤーの向きに応じた回転）
		Matrix4x4 rotY = MakeRotateYMatrix(transform.rotate.y);

		// 入力方向ベクトルをプレイヤーの向きに回転
		Vector3 rotatedDir = TransformVector(move_.direction, rotY);

		// Z方向を反転して前後の逆転を補正
		rotatedDir.z *= -1.0f;

		// 回転後の方向に沿って移動
		transform.translate.x += rotatedDir.x * currentSpeed;
		transform.translate.z += rotatedDir.z * currentSpeed;
	}


	// ----------------
	// 二段ジャンプ処理
	// ----------------
	// -------------------------------
	// 地面に接地していたらリセット
	// -------------------------------
	if (transform.translate.y <= jump_.kGroundHeight) {
		transform.translate.y = jump_.kGroundHeight;
		jump_.isJumping = false;
		jump_.velocity = 0.0f;
		jump_.jumpCount = 0;
		jump_.canJump_ = true;  // 接地時に再ジャンプを許可
		move_.hasDashed_ = false; // 接地時にダッシュ再許可
	}

	// -------------------------------
	// Spaceキーを押した → ジャンプ条件確認
	// -------------------------------
	if (jump_.canJump_ && Input::GetInstance()->PushKey(DIK_SPACE) &&
		jump_.jumpCount < jump_.kMaxJumpCount) {

		// ジャンプ開始
		jump_.velocity = jump_.kInitialVelocity;
		jump_.isJumping = true;
		jump_.jumpCount++;

		// 今はジャンプ許可しない（離されるまで）
		jump_.canJump_ = false;
	}

	// Spaceキーが離されたらジャンプを再許可
	if (!Input::GetInstance()->PushKey(DIK_SPACE)) {
		jump_.canJump_ = true;
	}

	// -------------------------------
	// ジャンプ中のY移動＆重力
	// -------------------------------
	if (jump_.isJumping) {
		transform.translate.y += jump_.velocity;
		jump_.velocity -= jump_.kGravity;
	}
}

void Player::HandleBullet()
{
	// Hキーで弾を1発発射
	// -------------------------------
	// 発射処理（Hキー）
	if (Input::GetInstance()->TriggerKey(DIK_H) && bullet_ == nullptr) {
		bullet_ = std::make_unique<PlayerBullet>(baseScene_);
		bullet_->SetTranslate(object3d_->GetTranslate());

		Vector3 forward = { 0.0f, 0.0f, 1.0f }; // プレイヤーの正面方向に仮固定
		bullet_->SetVelocity(forward * 0.5f);
		bullet_->Initialize();
		bullet_->SetCamera(camera_);
	}

	// 更新処理
	if (bullet_) {
		bullet_->Update();

		if (bullet_->IsDead()) {
			bullet_.reset(); // 4秒経ったら削除
		}
	}
}

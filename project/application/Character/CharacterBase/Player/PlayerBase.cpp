#include "PlayerBase.h"
#include <SceneManager.h>
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

	// ▼ここを毎回実行しない
	if (isFirstInitialize_) {
		transform.translate = { 0.0f, 0.0f, -20.0f };
		transform.rotate = { 0.0f, 0.0f,  0.0f };
		transform.scale = { 1.0f, 1.0f,  1.0f };
		isFirstInitialize_ = false;
	}

	// ここは常に現在のtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	SetCollider(this);
	SetPosition(transform.translate);
	sphere.radius = 2.0f;
	SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));
}

void PlayerBase::Update()
{
	// playerの基本となる動きの呼出し
	Move();

	GameOver();

	// ---- 死亡ビネットの進行 ----
	if (deathVig_.active) {
		const float dt = 1.0f / 60.0f;
		deathVig_.scale = std::min(deathVig_.targetScale, deathVig_.scale + deathVig_.speedScale * dt);
		deathVig_.power = std::min(deathVig_.targetPower, deathVig_.power + deathVig_.speedPower * dt);

		auto pe = PostEffectManager::GetInstance();
		pe->SetType(PostEffectType::Vignette);
		pe->SetVignetteColor(deathVig_.color);
		pe->SetVignetteScale(deathVig_.scale);
		pe->SetVignettePower(deathVig_.power);

		// ---- defeat.png のフェードイン管理 ----
		if (defeat_.active) {
			// 初回初期化
			if (!defeat_.initialized) {
				defeat_.sprite = std::make_unique<Sprite>();
				defeat_.sprite->Initialize("Resources/defeat.png");
				defeat_.sprite->SetAnchorPoint({ 0.5f, 0.5f });
				defeat_.sprite->SetPosition({ 1280.0f * 0.5f, 720.0f * 0.5f }); // 画面中央
				// サイズ指定が必要なら（画像原寸で良ければ不要）
				defeat_.sprite->SetSize({ 1280.0f * 0.9f, 720.0f * 0.9f });
				defeat_.sprite->SetColor({ 1.0f,0.3f,0.3f,1.0f });
				defeat_.initialized = true;
			}

			defeat_.timer += dt;
			// 遅延後にフェードイン
			float t = std::max(0.0f, (defeat_.timer - defeat_.delay) / std::max(0.001f, defeat_.fadeSec));
			defeat_.alpha = std::clamp(t, 0.0f, 1.0f);

			// アルファ適用（RGBはそのまま、Aだけ上げる）
			defeat_.sprite->SetColor({ 1.0f, 1.0f, 1.0f, defeat_.alpha });
			defeat_.sprite->Update();
		}
		// ---- 暗転完了 → TITLEへ ----
		// ※ defeat.png が出きってから 2秒後に戻る
		static float returnTimer = 0.0f;
		if (deathVig_.scale >= deathVig_.targetScale &&
			deathVig_.power >= deathVig_.targetPower &&
			defeat_.alpha >= 1.0f) {

			returnTimer += dt;
			if (returnTimer > 2.0f) {
				SceneManager::GetInstance()->ChangeScene("TITLE");
				returnTimer = 0.0f; // 念のためリセット
			}
		}
	}

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

void PlayerBase::ForeGroundDraw()
{
	if (defeat_.active && defeat_.initialized && defeat_.alpha > 0.0f) {
		defeat_.sprite->Draw();
	}
}

void PlayerBase::Draw()
{
	// SphereCollider の描画
	//SphereCollider::Draw();
}

void PlayerBase::SkinningDraw()
{
	object3d_->Draw();
}

void PlayerBase::ParticleDraw()
{
}

void PlayerBase::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);

	isGameOver = true;
	move_.isDashing = false;
	move_.dashTimer = 0.0f;
	jump_.isJumping = false;
	jump_.velocity = 0.0f;
}

void PlayerBase::Move()
{
	// ゲームオーバー中は入力も移動も受け付けない＆アニメも変えない
	if (isGameOver) {
		move_.direction = { 0,0,0 };
		return;
	}

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

	if (animCtrl_) {
		if (isMoving) {
			PlayAnimKey(PlayerAnimKey::RunWeapon);   // 走り
		}
		else {
			PlayAnimKey(PlayerAnimKey::Idle);        // 待機
		}
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

void PlayerBase::GameOver()
{
	if (isGameOver && !deathAnimLatched_) {
		PlayAnimKey(PlayerAnimKey::Death);

		// ここで1回だけ再生に切替
		object3d_->SetAnimationLoop(false);   // ループOFF
		object3d_->SetAnimationTime(0.0f);    // 先頭から
		// object3d_->SetAnimationPlaybackRate(1.0f); // 必要なら

		//　ここでビネット開始
		BeginDeathVignette(/*color=*/{ 0,0,0 }, /*startScale=*/0.0f, /*startPower=*/0.0f,
			/*speed=*/0.6f, /*targetScale=*/1.25f, /*targetPower=*/2.2f);

		// defeat.png 表示も開始（遅延付きでフェード）
		defeat_.active = true;
		defeat_.initialized = false;
		defeat_.alpha = 0.0f;
		defeat_.timer = 0.0f;

		deathAnimLatched_ = true;
	}

}

void PlayerBase::ChangeModel(const char* modelName)
{
	ModelManager::GetInstance()->LoadModel(modelName);
	object3d_->SetModel(modelName);
}

void PlayerBase::PlayAnimKey(PlayerAnimKey key)
{
	if (!animCtrl_) return;
	SetAnimationIfChanged(animCtrl_->Resolve(key));
}

void PlayerBase::SetAnimationIfChanged(const std::string& name)
{
	if (name.empty()) return;              // 安全ガード
	if (currentAnimationName_ == name) return;
	object3d_->SetAnimation(name);
	currentAnimationName_ = name;
}

void PlayerBase::BeginDeathVignette(const Vector3& color, float startScale, float startPower, float speed, float targetScale, float targetPower)
{
	deathVig_.active = true;
	deathVig_.color = color;

	deathVig_.scale = startScale;
	deathVig_.power = startPower;
	deathVig_.speedScale = speed;
	deathVig_.speedPower = speed;
	deathVig_.targetScale = targetScale;
	deathVig_.targetPower = targetPower;

	// 初期適用
	PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
	PostEffectManager::GetInstance()->SetVignetteColor(deathVig_.color);
	PostEffectManager::GetInstance()->SetVignetteScale(deathVig_.scale);
	PostEffectManager::GetInstance()->SetVignettePower(deathVig_.power);
}

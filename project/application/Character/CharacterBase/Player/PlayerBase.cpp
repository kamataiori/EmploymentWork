#include "PlayerBase.h"
#include "PlayerWeaponOBB.h"
#include <FollowCamera.h>
#include "TimeManager.h"
#include <PostEffectManager.h>

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

	// === HPバー初期化（Player 左下） ===
	hpBarBG_ = std::make_unique<Sprite>();
	hpBarFill_ = std::make_unique<Sprite>();

	hpBarBG_->Initialize("Resources/hp.png");
	hpBarFill_->Initialize("Resources/hp.png");

	// 背景は少し暗め
	hpBarBG_->SetColor({ 0.2f, 0.2f, 0.2f, 0.8f });
	// 本体は白（テクスチャ色そのまま）
	hpBarFill_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 左下に固定したいのでアンカーポイントを「左下 (0,1)」にしておく
	hpBarBG_->SetAnchorPoint({ 0.0f, 1.0f });
	hpBarFill_->SetAnchorPoint({ 0.0f, 1.0f });


	//poweder = std::make_unique<ParticleManager>();
	//poweder->Initialize(ParticleManager::VertexDataType::Plane);

	//// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	//poweder->LoadAllPresets();
}

void PlayerBase::Update()
{
	// ===== Δt（スロー対応） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// ロックの減衰
	if (animLockTimer_ > 0.0f) {
		animLockTimer_ -= dt;
		if (animLockTimer_ <= 0.0f) {
			animLockTimer_ = 0.0f;
			currentAnimPriority_ = 0;
			// 止まっている場合に備えて一度Idleを要求しておく
			RequestAnimKey(PlayerAnimKey::Idle, 0);
		}
	}

	// 被弾無敵タイマーの減衰
	if (hitTimer_ > 0.0f) {
		hitTimer_ -= dt;
		if (hitTimer_ < 0.0f) {
			hitTimer_ = 0.0f;
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


	// === HPバー更新 ===
	if (hpBarBG_ && hpBarFill_) {
		const float ratio =
			(kMaxHP_ > 0) ? std::clamp(hp_ / float(kMaxHP_), 0.0f, 1.0f) : 0.0f;

		const float winW = 1280.0f;
		const float winH = 720.0f;

		const float maxW = hpBarMaxWidth_;
		const float curW = maxW * ratio;
		const float left = hpBarMarginLeft_;
		const float bottom = winH - hpBarMarginBottom_;

		// 背景は常に最大幅
		hpBarBG_->SetSize({ maxW, hpBarHeight_ });
		hpBarBG_->SetPosition({ left, bottom });
		hpBarBG_->Update();

		// 本体は現在HPに応じた幅
		hpBarFill_->SetSize({ curW, hpBarHeight_ });
		hpBarFill_->SetPosition({ left, bottom });
		hpBarFill_->Update();

		// ===============================
	    //  HP に応じたビネット演出
	    //  ratio が 0.5 → 0.0 で徐々に強くする
	    // ===============================
		if (ratio < 0.7f) {
			// 0.5 の時 0、0.0 の時 1 になる係数
			float t = (0.7f - ratio) / 0.7f;
			t = std::clamp(t, 0.0f, 1.0f);

			// 補間用ヘルパ（なければ自前で書いてOK）
			auto Lerp = [](float a, float b, float t) {
				return a + (b - a) * t;
				};

			// --- パラメータ設定（好みで調整してOK） ---
			// Power: ビネットの濃さ
			float minPower = 0.1f;
			float maxPower = 0.4f;
			float power = Lerp(minPower, maxPower, t);

			// Scale: 画面のどこまでビネットが広がるか
			float minScale = 0.2f;
			float maxScale = 0.6f;
			float scale = Lerp(minScale, maxScale, t);

			// Color: 濃い赤にしていく
			Vector3 minColor{ 0.4f, 0.0f, 0.0f }; // 暗めの赤
			Vector3 maxColor{ 1.0f, 0.0f, 0.0f }; // 明るく強い赤
			Vector3 color{
				Lerp(minColor.x, maxColor.x, t),
				Lerp(minColor.y, maxColor.y, t),
				Lerp(minColor.z, maxColor.z, t)
			};

			// ビネット有効化 & パラメータ反映
			PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
			PostEffectManager::GetInstance()->SetVignettePower(power);
			//PostEffectManager::GetInstance()->SetVignetteScale(scale);
			PostEffectManager::GetInstance()->SetVignetteColor(color);
		}
		else {
			// HP が半分以上ならビネットを切る（通常描画に戻す）
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
		}
	}


	/*poweder->Update();*/
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
	// === HPバー描画（左下） ===
	if (hpBarBG_ && hpBarFill_) {
		hpBarBG_->Draw();
		hpBarFill_->Draw();
	}
}

void PlayerBase::AnimationDraw()
{
	object3d_->Draw();
}

void PlayerBase::ParticleDraw()
{
	/*poweder->Draw();*/
}

void PlayerBase::OnCollision()
{
	// 相手が敵でなければ何もしない（武器や弾との衝突を無視）
	/*if (other->GetTypeID() != (uint32_t)CollisionTypeIdDef::kEnemy) {
		return;
	}*/

	// 多段ヒット防止（無敵時間中なら何もしない）
	if (hitTimer_ > 0.0f) {
		return;
	}

	// ====== HP減少処理 ======
	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	// ヒットした瞬間にカメラシェイク
	//if (cameraEffectController_) {
	//	// 手軽なシンプル版シェイク
	//	// (duration: 0.2秒, 振れ幅: 0.25, 全方向)
	//	cameraEffectController_->StartSimpleShake(
	//		0.02f,
	//		0.25f,
	//		CameraEffectController::ShakeMode::Horizontal
	//	);
	//}


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
	// ===== Δt（回転のスムージング用） =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

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

void PlayerBase::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	/*if (poweder) {
		poweder->SetCamera(camera);
	}*/

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
	// ロック中は同じか低い優先度のリクエストを無視
	if (animLockTimer_ > 0.0f && priority <= currentAnimPriority_) {
		return;
	}

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

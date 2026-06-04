#include "MinionEnemy.h"
#include "engine/TimeManager.h"
#include "engine/3d/Camera/Camera.h"
#include "engine/UI/IDamagePopupSink.h"
#include <WinApp.h>
#include <cmath>

static float WrapDeltaRad(float a) {
	while (a > 3.1415926535f) a -= 6.283185307f;
	while (a < -3.1415926535f) a += 6.283185307f;
	return a;
}

static float LerpAngleRad(float from, float to, float t) {
	const float d = WrapDeltaRad(to - from);
	return from + d * t;
}

static Vector3 NormalizeSafe(const Vector3& v)
{
	float len = Length(v);
	if (len < 1e-6f) return { 0,0,1 };
	return v / len;
}

MinionEnemy::MinionEnemy(BaseScene* scene)
	: ObjectBase(scene)
{
}

void MinionEnemy::InitializeMinion(const Vector3& spawnPos)
{
	ModelManager::GetInstance()->LoadModel("minion.obj");

	object3d_->Initialize();
	object3d_->SetModel("minion.obj");

	// 立ち位置を記憶（モデルの足元が地面に来るよう少し持ち上げる）
	surfacePos_ = spawnPos;
	surfacePos_.y += groundOffsetY_;
	groundY_ = surfacePos_.y;

	// 地面の下からスタート
	transform.translate = surfacePos_;
	transform.translate.y -= spawnDepth_;
	transform.rotate = { 0,0,0 };
	transform.scale = { 1,1,1 };

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	phase_ = Phase::Spawn;
	isDead_ = false;
	attackActive_ = false;
	actedThisRound_ = false;
	hp_ = kMaxHP_;
	shockwaveTimer_ = 0.0f;
	explodeTimer_ = 0.0f;

	// 撃破時の爆発パーティクル（この個体専用）。Explosion.json を読み込んでおく。
	particles_ = std::make_unique<ParticleManager>();
	particles_->Initialize(VertexDataType::Plane);
	particles_->LoadAllPresets();
	if (camera_) {
		particles_->SetCamera(camera_);
	}

	// 頭上HPピップ（HP.pngを実寸サイズで kMaxHP_ 枚作成し、横並びで表示する）
	// 位置はUpdate内で毎フレーム頭上に追従させる
	for (int i = 0; i < kMaxHP_; ++i) {
		hpPips_[i] = std::make_unique<Sprite>();
		hpPips_[i]->Initialize("Resources/HP.png");
		// Initialize内のAdjustTextureSizeで自動的にテクスチャ実寸になるが、
		// 明示的にも実寸を再指定して拡大されないことを保証する
		hpPips_[i]->SetSize({ kHpPipWidth_, kHpPipHeight_ });
		hpPips_[i]->SetAnchorPoint({ 0.0f, 0.5f }); // 左端基準で横並べしやすくする
		hpPips_[i]->SetColor(kHpPipColor_);           // ユーザー指定の緑で乗算
	}
	hpBarVisible_ = false; // 出現演出中は非表示。地表に着いたら表示

	// コライダー（Sphere）
	multiCollider_->Clear();

	Sphere sp{};
	sp.center = transform.translate;
	sp.radius = radius_;
	multiCollider_->AddSphere(sp);
	multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });
}

void MinionEnemy::BeginAttack(const Vector3& targetPos)
{
	// 待機中以外から呼ばれても無視する
	if (phase_ != Phase::Idle) return;

	// 突進先を確定（XZのみ採用。高さは自分の地面に合わせる）
	chargeTargetXZ_ = { targetPos.x, groundY_, targetPos.z };
	transform.translate.y = groundY_; // 待機のホバー揺れ位置から地面へ戻して突進開始
	phase_ = Phase::Charge;
	attackActive_ = true;
}

void MinionEnemy::Update()
{
	if (isDead_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();

	switch (phase_)
	{
	case Phase::Spawn:
	{
		// 地面の下から上昇
		transform.translate.y += spawnRiseSpeed_ * dt;

		// 地面の高さに到達したら待機へ
		if (transform.translate.y >= surfacePos_.y) {
			transform.translate.y = surfacePos_.y;
			phase_ = Phase::Idle;
		}
		break;
	}

	case Phase::Idle:
	{
		// 生きた待機：完全静止にせず、ゆるくホバー上下動しつつプレイヤーを向く。
		// Enemy の BeginAttack を待つ点は同じ。
		idleTime_ += dt;

		// ホバー上下動（groundY_ から上方向にだけ揺れる）
		const float bob = (1.0f - std::cos(idleTime_ * idleBobSpeed_)) * 0.5f * idleBobAmplitude_;
		transform.translate.y = groundY_ + bob;

		// ゆっくりプレイヤーの方を向く
		if (playerTarget_) {
			Vector3 toPlayer = playerTarget_->translate - transform.translate;
			toPlayer.y = 0.0f;
			if (Length(toPlayer) > 1e-4f) {
				Vector3 dir = NormalizeSafe(toPlayer);
				const float desiredYaw = std::atan2(dir.x, dir.z);
				transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, idleTurnLerp_);
			}
		}
		break;
	}

	case Phase::Charge:
	{
		// 取得済みのプレイヤー位置へ突進
		Vector3 toTarget = chargeTargetXZ_ - transform.translate;
		toTarget.y = 0.0f;
		float dist = Length(toTarget);

		if (dist <= reachThreshold_) {
			// 到達 → 一旦止まって上昇へ
			transform.translate.x = chargeTargetXZ_.x;
			transform.translate.z = chargeTargetXZ_.z;
			phase_ = Phase::Rise;
		}
		else {
			Vector3 dir = NormalizeSafe(toTarget);

			// 進行方向を向く
			float desiredYaw = std::atan2(dir.x, dir.z);
			transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, turnLerp_);

			// 行き過ぎないようにクランプ
			float step = chargeSpeed_ * dt;
			if (step > dist) step = dist;
			transform.translate.x += dir.x * step;
			transform.translate.z += dir.z * step;
		}
		break;
	}

	case Phase::Rise:
	{
		// 突進先でY座標上に上昇
		transform.translate.y += liftSpeed_ * dt;
		if (transform.translate.y >= groundY_ + riseHeight_) {
			transform.translate.y = groundY_ + riseHeight_;
			phase_ = Phase::SpinTop;
			spinAccumulated_ = 0.0f;
		}
		break;
	}

	case Phase::SpinTop:
	{
		// てっぺんで停止したまま高速回転（spinTurns_ 周）
		float d = spinSpeed_ * dt;
		transform.rotate.y += d;
		spinAccumulated_ += d;
		if (spinAccumulated_ >= spinTurns_ * 2.0f * 3.1415926535f) {
			phase_ = Phase::Slam;
		}
		break;
	}

	case Phase::Slam:
	{
		// 急降下（回転はしない）
		transform.translate.y -= slamSpeed_ * dt;
		if (transform.translate.y <= groundY_) {
			transform.translate.y = groundY_;
			transform.scale = impactScale_; // 着地の衝撃で潰れる（Recoverで戻す）
			phase_ = Phase::Shockwave;
			shockwaveTimer_ = 0.0f;
		}
		break;
	}

	case Phase::Shockwave:
	{
		// 着地点に範囲ダメージ（下のコライダー更新で半径を拡大）
		shockwaveTimer_ += dt;
		if (shockwaveTimer_ >= shockwaveDuration_) {
			// 衝撃波おわり → 硬直(Recover)へ。
			// attackActive_ をここで false にするので、コーディネーターは
			// 従来どおりこのタイミングで次の雑魚敵を選ぶ（突進間隔は変わらない）。
			phase_ = Phase::Recover;
			recoverTimer_ = 0.0f;
			attackActive_ = false;
		}
		break;
	}

	case Phase::Recover:
	{
		// 急降下後の硬直：無防備で動けない＝プレイヤーの反撃チャンス。
		// 潰れスケールの戻りは硬直より短い scaleRecoverDuration_ で素早く立ち直らせる。
		recoverTimer_ += dt;
		float t = recoverTimer_ / scaleRecoverDuration_;
		if (t > 1.0f) t = 1.0f;
		transform.scale.x = impactScale_.x + (1.0f - impactScale_.x) * t;
		transform.scale.y = impactScale_.y + (1.0f - impactScale_.y) * t;
		transform.scale.z = impactScale_.z + (1.0f - impactScale_.z) * t;

		if (recoverTimer_ >= recoverDuration_) {
			transform.scale = { 1.0f, 1.0f, 1.0f };
			idleTime_ = 0.0f;       // 待機演出を最初から
			phase_ = Phase::Idle;
		}
		break;
	}

	case Phase::Hit:
	{
		// 固まったまま小刻みにシェイク → 時間切れで死亡(削除)
		// X/Y/Z で周波数と位相をずらして合成し、機械的にならない揺れにする
		hitTimer_ += dt;

		const float t = hitTimer_;
		const float ox = std::sin(t * kHitShakeFreqX_) * kHitShakeAmplitude_;
		const float oz = std::sin(t * kHitShakeFreqX_ * kHitShakeFreqZRatio_ + kHitShakePhaseZ_)
			* kHitShakeAmplitude_;
		const float oy = std::sin(t * kHitShakeFreqX_ * kHitShakeFreqYRatio_ + kHitShakePhaseY_)
			* kHitShakeAmplitude_ * kHitShakeYAttenuation_;

		transform.translate.x = hitBasePos_.x + ox;
		transform.translate.y = hitBasePos_.y + oy;
		transform.translate.z = hitBasePos_.z + oz;

		if (hitTimer_ >= kHitDuration_) {
			// シェイク終了 → 爆発演出へ。爆発パーティクルを1回だけ発生させる。
			phase_ = Phase::Explode;
			explodeTimer_ = 0.0f;

			if (particles_) {
				Transform burst{};
				burst.scale = { 1.0f, 1.0f, 1.0f };
				burst.rotate = { 0.0f, 0.0f, 0.0f };
				burst.translate = hitBasePos_;
				burst.translate.y += kExplosionOffsetY_; // 体の中心あたりから噴き出す
				// EmitterInstance 経路で出す（EmitByPresetName の particleList 経路は描画されないため）。
				particles_->EmitPreset(kExplosionPreset_, burst);
			}
		}
		break;
	}

	case Phase::Explode:
	{
		// 爆発パーティクルを再生しきってから消滅する（モデルは描画しない）。
		explodeTimer_ += dt;
		if (explodeTimer_ >= kExplosionLifetime_) {
			isDead_ = true;
		}
		break;
	}
	}

	// コライダー更新（衝撃波中のみ半径を拡大して範囲ダメージにする）
	// 死亡演出中（Hit/Explode）は当たり判定を消して、瀕死の個体が触れて事故らないようにする。
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = transform.translate;
	if (phase_ == Phase::Hit || phase_ == Phase::Explode) {
		sp.radius = 0.0f;
	}
	else {
		sp.radius = (phase_ == Phase::Shockwave) ? shockwaveRadius_ : radius_;
	}

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

	// 頭上HPバーの追従と表示更新
	UpdateHpBar();

	// 爆発パーティクルの更新（発生後、消滅まで毎フレーム進める）
	if (particles_) {
		particles_->Update();
	}
}

void MinionEnemy::UpdateHpBar()
{
	// 出現演出中・被弾死亡演出中はバーを隠す（地中から伸びるバーは不自然なため）
	const bool visible = !isDead_
		&& phase_ != Phase::Spawn
		&& phase_ != Phase::Hit
		&& phase_ != Phase::Explode;
	hpBarVisible_ = visible;
	if (!visible) return;

	// カメラ未設定なら投影できないので位置更新はスキップ
	if (!camera_) return;

	// 頭上ワールド位置 → クリップ空間
	const Matrix4x4& vp = camera_->GetViewProjectionMatrix();
	const Vector3 headPos = {
		transform.translate.x,
		transform.translate.y + kHpBarHeadOffsetY_,
		transform.translate.z,
	};
	const float cx = headPos.x * vp.m[0][0] + headPos.y * vp.m[1][0] + headPos.z * vp.m[2][0] + vp.m[3][0];
	const float cy = headPos.x * vp.m[0][1] + headPos.y * vp.m[1][1] + headPos.z * vp.m[2][1] + vp.m[3][1];
	const float cw = headPos.x * vp.m[0][3] + headPos.y * vp.m[1][3] + headPos.z * vp.m[2][3] + vp.m[3][3];

	// カメラ背後（w<=0）の場合は描画しない
	if (cw <= kHpBarMinClipW_) {
		hpBarVisible_ = false;
		return;
	}

	const float ndcX = cx / cw;
	const float ndcY = cy / cw;

	const float screenW = static_cast<float>(WinApp::kClientWidth);
	const float screenH = static_cast<float>(WinApp::kClientHeight);
	const float sx = (ndcX + 1.0f) * 0.5f * screenW;
	const float sy = (1.0f - ndcY) * 0.5f * screenH;

	// 全ピップを並べたときの合計幅から、中央寄せの左端を求める
	const float barWidth = kMaxHP_ * kHpPipWidth_ + (kMaxHP_ - 1) * kHpPipSpacing_;
	const float leftX = sx - barWidth * 0.5f;

	// 各ピップを横に並べる（実寸維持。サイズは触らない）
	for (int i = 0; i < kMaxHP_; ++i) {
		const float px = leftX + i * (kHpPipWidth_ + kHpPipSpacing_);
		hpPips_[i]->SetPosition({ px, sy });
		hpPips_[i]->Update();
	}
}

void MinionEnemy::Draw()
{
	multiCollider_->Draw();

	// 爆発演出中はモデルを消す（爆発して消滅した見た目にする）
	if (phase_ == Phase::Explode) return;

	object3d_->Draw();
}

void MinionEnemy::ParticleDraw()
{
	if (particles_) {
		particles_->Draw();
	}
}

void MinionEnemy::ForeGroundDraw()
{
	// HPピップは2Dスプライトなので、前景パスで個体ごとに描画する
	// 残HP分だけ左から描く（被弾でhp_が減ると右端から欠けていく）
	if (!hpBarVisible_) return;
	for (int i = 0; i < hp_ && i < kMaxHP_; ++i) {
		hpPips_[i]->Draw();
	}
}

void MinionEnemy::OnCollision(const CollisionInfo& info)
{
	// 既にヒット/爆発演出に入っている、または死亡確定後は無視（同じ攻撃で多段ヒットしないため）
	if (isDead_ || phase_ == Phase::Hit || phase_ == Phase::Explode) return;

	auto other = static_cast<CollisionTypeIdDef>(info.otherType);

	if (other == CollisionTypeIdDef::kPlayerWeapon ||
		other == CollisionTypeIdDef::kPlayerAttack ||
		other == CollisionTypeIdDef::PlayerBullet)
	{
		ApplyDamage(kDamagePerHit_);
	}
}

void MinionEnemy::ApplyDamage(int amount)
{
	// 既にヒット/爆発演出に入っている、または死亡確定後は無視（多段ヒット防止）
	if (isDead_ || phase_ == Phase::Hit || phase_ == Phase::Explode) return;

	// 与ダメージ量を雑魚の右上にポップアップ表示
	if (damageSink_ && amount > 0) {
		damageSink_->SpawnDamage(GetTargetCenter(), amount);
	}

	// 被弾した瞬間に火花を飛び散らせる（描画される EmitPreset 経路で出す）
	if (particles_) {
		Transform spark{};
		spark.scale = { 1.0f, 1.0f, 1.0f };
		spark.rotate = { 0.0f, 0.0f, 0.0f };
		spark.translate = transform.translate;
		spark.translate.y += kHitSparkOffsetY_;
		particles_->EmitPreset(kHitSparkPreset_, spark);
	}

	hp_ -= amount;
	if (hp_ <= 0) {
		// すぐ消さず、ヒットストップ演出（固まり＋小刻みシェイク）へ
		phase_ = Phase::Hit;
		hitTimer_ = 0.0f;
		hitBasePos_ = transform.translate; // シェイクの基準点
		attackActive_ = false;             // 進行中の攻撃も停止
	}
}

void MinionEnemy::SetCamera(Camera* camera)
{
	ObjectBase::SetCamera(camera);
	if (particles_) {
		particles_->SetCamera(camera);
	}
}

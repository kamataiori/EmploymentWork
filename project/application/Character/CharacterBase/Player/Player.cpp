#include "Player.h"
#include "PlayerWeapon.h"
#include <FollowCamera.h>
#include "engine/TimeManager.h"
#include "ParticleManager.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// 前方宣言した ParticleManager / ParticleEmitterInstance がここでは完全型になるため、
// unique_ptr メンバを安全に破棄できる。
Player::~Player() = default;

void Player::Initialize()
{
	// object3dの初期化
	object3d_->Initialize();


	object3d_->SetModel("Warrior.gltf");
	// アニメ切替の補間（コンボ用に短め）
	object3d_->SetBlendDuration(0.10f);

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
	// 相手の種別を見て被弾を判定するため Ex 版で登録する
	// （SetHitCallback だと種別情報が捨てられ、何に当たっても無条件で被弾する）
	multiCollider_->SetHitCallbackEx(
		[this](const CollisionInfo& info) { this->OnCollision(info); });

	// 移動コンポーネントを生成し、本体の Transform と接地情報を共有する
	mover_ = std::make_unique<PlayerMover>();
	mover_->Initialize(&transform);
	mover_->SetGroundParams(sphereRadius_, colliderOffset_);
	mover_->SetCamera(camera_); // camera_ が後で入るなら SetCamera() 側でも呼ぶ

	sword_ = std::make_unique<Sword>(baseScene_);
	sword_->Initialize();
	sword_->SetCamera(camera_); // camera_ が後で入るなら SetCamera() 側でも呼ぶ
	sword_->SetPlayerTransform(&transform);
	// ボーン名は実際の名前に合わせて修正が必要
	sword_->AttachTo(object3d_.get(), "Fist.R");

	// 武器（攻撃・スキルの調停）を生成。スキルが駆動する剣を注入してから初期化する。
	weapon_ = std::make_unique<PlayerWeapon>();
	weapon_->SetOwner(this);
	weapon_->SetPlayerTransform(&transform);
	static_cast<PlayerWeapon*>(weapon_.get())->SetSword(sword_.get());
	weapon_->Initialize();


	// ===== 通常状態の剣オーラ用パーティクル =====
	// 何もしていないときに剣から立ち上るオーラ。持続エミッタを1つだけ生成して保持し、
	// 毎フレーム位置の追従と Play/Stop の切り替えだけ行う。
	swordAura_ = std::make_unique<ParticleManager>();
	swordAura_->Initialize(VertexDataType::Plane);
	swordAura_->LoadAllPresets(); // Resources/Particle/SwordAura.json を含めて読み込む
	if (camera_) {
		swordAura_->SetCamera(camera_);
	}
	// 持続エミッタを生成（repeat=true のプリセット）。最初は止めておき、通常状態で再生する。
	auraEmitter_ = swordAura_->EmitPreset(kSwordAuraPresetName_, transform);
	if (auraEmitter_) {
		auraEmitter_->Stop();
	}
	auraPlaying_ = false;
}

void Player::SetEnemyTargetProvider(IEnemyTargetProvider* provider)
{
	// 武器（調停役）経由でアルティメットへ供給元を渡す。
	if (weapon_) weapon_->SetEnemyTargetProvider(provider);
}

void Player::Update()
{
	// ===== Δt =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();

	// ===== デバッグ：Xキーで即死 =====
	if (!isDead_ && Input::GetInstance()->TriggerKey(DIK_X)) {
		hp_ = 0;
	}

	// ===== 死亡突入（hp_が0になった瞬間に1回だけ） =====
	if (!isDead_ && hp_ <= 0) {
		isDead_ = true;
		deathTimer_ = kDeathDuration_;
		inputLocked_ = true;
		object3d_->SetAnimationSpeed(1.0f); // 攻撃の速度倍率が残らないよう等倍に戻す
		object3d_->SetAnimationOneShot("Death");
	}

	// ===== 死亡中：タイマーだけ進めてアニメを回す =====
	if (isDead_) {
		deathTimer_ -= dt;
		if (deathTimer_ < 0.0f) deathTimer_ = 0.0f;

		object3d_->SetTranslate(transform.translate);
		object3d_->SetRotate(transform.rotate);
		object3d_->SetScale(transform.scale);
		object3d_->Update();

		if (sword_) {
			sword_->SetHitEnabled(false);
			sword_->Update();
		}

		// 死亡中はオーラを止める（残っている粒子はフェードしながら消える）
		if (swordAura_) {
			if (auraEmitter_ && auraPlaying_) {
				auraEmitter_->Stop();
				auraPlaying_ = false;
			}
			swordAura_->Update();
		}

		return;
	}

	// ロックの減衰
	if (animaLockTimer_ > 0.0f) {
		animaLockTimer_ -= dt;
		if (animaLockTimer_ <= 0.0f) {
			animaLockTimer_ = 0.0f;
			currentAnimaPriority_ = 0;
		}
	}

	// playerの基本となる動き（移動コンポーネントに委譲）
	mover_->Update(inputLocked_);

	bool weaponAttacking = false;     // 攻撃モーション中（アニメ上書き禁止用）

	if (weapon_) {
		// 武器の更新（通常攻撃の進行＋スキルの駆動はこの中で完結する）
		weapon_->Update();

		if (!inputLocked_) {
			weapon_->NormalAttack();
			weapon_->Skill();
			weapon_->Ultimate();
		}

		if (auto w = dynamic_cast<PlayerWeapon*>(weapon_.get())) {
			weaponAttacking = w->IsAttacking();
		}
	}

	// object3d_->Update() の前にアニメーションを決める
	if (!IsAnimaLocked() && !weaponAttacking) {
		if (mover_->IsMoving()) {
			RequestAnimaKey(PlayerAnimKey::RunWeapon, 0);
		}
		else {
			RequestAnimaKey(PlayerAnimKey::Idle, 0);
		}
	}

	// --- 先に Player 本体を更新する ---
	colliderTranslate_ = transform.translate + colliderOffset_;

	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	// Player 本体更新後に Sword を更新
	// （当たり判定ON/OFF・サイズ・ワールド配置は weapon_/スキルが既に設定済み。
	//   ここではその状態を反映するだけ）
	if (sword_) {
		sword_->Update();
	}

	// Player 本体コライダー更新
	isCollided_ = false;
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;

	// ===== E スキル（回転斬り）発動中だけ剣からオーラを出す =====
	bool eSkillActive = false;
	if (auto w = dynamic_cast<PlayerWeapon*>(weapon_.get())) {
		eSkillActive = w->IsESkillActive();
	}
	const bool auraShouldPlay = !isDead_ && eSkillActive;
	if (swordAura_ && auraEmitter_) {
		// 剣そのもののワールド座標へエミッタを追従させる
		// （手に持っている間も、スキルで切り離して振り回している間も剣の位置に付いてくる）
		Transform auraTf = transform;
		if (sword_) {
			auraTf.translate = sword_->GetWorldPosition();
		}
		auraEmitter_->SetTransform(auraTf);

		// Play() は emitTimer をリセットするため、状態が切り替わった瞬間だけ呼ぶ
		if (auraShouldPlay && !auraPlaying_) {
			auraEmitter_->Play();
			auraPlaying_ = true;
		}
		else if (!auraShouldPlay && auraPlaying_) {
			auraEmitter_->Stop();
			auraPlaying_ = false;
		}
	}
	if (swordAura_) {
		swordAura_->Update();
	}
}

void Player::UpdateVisual()
{
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);
	object3d_->Update();

	if (sword_) {
		sword_->SetHitEnabled(false);
		sword_->Update();
	}
}

void Player::BackGroundDraw()
{
}

void Player::Draw()
{
	multiCollider_->Draw();
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
	if (swordAura_) {
		swordAura_->Draw();
	}
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

	if (isDead_) return; // 死亡中は被弾無視
	if (invincible_) return; // 無敵中（アルティメット突進中など）は被弾無視

	hp_ -= kDamagePerHit_;
	if (hp_ < 0) hp_ = 0;

	isCollided_ = true;

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

void Player::OnCollision(const CollisionInfo& info)
{
	// 敵グループ（敵本体・雑魚・敵弾・範囲攻撃）からの接触のみ被弾する。
	// プレイヤー自身の武器や弾（kPlayerWeapon 等）では被弾しない。
	const auto otherType = static_cast<CollisionTypeIdDef>(info.otherType);
	if (GetGroup(otherType) == CollisionGroup::Enemy) {
		OnCollision();
	}
}

void Player::SetCamera(Camera* camera)
{
	// まずは ObjectBase 側の処理（camera_ と object3d_ にセット）
	ObjectBase::SetCamera(camera);

	if (mover_) mover_->SetCamera(camera);

	sword_->SetCamera(camera);

	// パーティクル側にも同じカメラを渡す
	if (swordAura_) {
		swordAura_->SetCamera(camera);
	}

	// 必要なら武器や他のオブジェクトにもここで渡せる
	// if (weapon_) { weapon_->SetCamera(camera); } みたいな感じで拡張可能
}

void Player::PlayAnimaKey(PlayerAnimKey key)
{
	SetAnimationIfChanged(animaCtrl_.Resolve(key));
}

void Player::RequestAnimaKey(PlayerAnimKey key, int priority, float lockSec, float speed)
{
	// 低い優先度からの上書きは禁止（攻撃中に移動で潰さない）
	if (priority < currentAnimaPriority_) return;

	PlayAnimaKey(key);
	currentAnimaPriority_ = priority;

	// このアニメの再生速度を反映（移動系は既定の 1.0 で等倍に戻る）
	object3d_->SetAnimationSpeed(speed);

	// ロックは長い方を採用で上書き
	if (lockSec > 0.0f) {
		if (animaLockTimer_ < lockSec) animaLockTimer_ = lockSec;
	}
}

void Player::SetAnimationIfChanged(const std::string& name)
{
	if (name.empty()) return;              // 安全ガード
	if (currentAnimationName_ == name) return;

	// 攻撃系はワンショット再生にして、ブレンドで繋ぐ
	if (name.rfind("Attack", 0) == 0 || name.rfind("Skill", 0) == 0 || name.rfind("Ultimate", 0) == 0) {
		object3d_->SetAnimationOneShot(name);
	}
	else {
		object3d_->SetAnimation(name);
	}
	currentAnimationName_ = name;
}

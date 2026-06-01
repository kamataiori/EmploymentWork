#include "Player.h"
#include "PlayerWeapon.h"
#include <FollowCamera.h>
#include "engine/TimeManager.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

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

	weapon_ = std::make_unique<PlayerWeapon>();
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


	//poweder = std::make_unique<ParticleManager>();
	//poweder->Initialize(ParticleManager::VertexDataType::Plane);

	//// Resources/Particle/*.json を読み込んでおく（fire.json を想定）
	//poweder->LoadAllPresets();
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
	bool weaponHitActive = false;     // 通常攻撃の当たり判定を出してよい区間
	bool weaponSkillActive = false;   // スキル(回転斬り)発動中か
	float weaponSkillProgress = 0.0f; // スキルの進行度 0〜1（剣の弧の位置計算用）

	if (weapon_) {
		weapon_->Update();

		if (!inputLocked_) {
			weapon_->NormalAttack();
			weapon_->Skill();
			weapon_->Ultimate();
		}

		if (auto w = dynamic_cast<PlayerWeapon*>(weapon_.get())) {
			weaponAttacking = w->IsAttacking();
			weaponHitActive = w->IsHitActive();
			weaponSkillActive = w->IsSkillActive();
			weaponSkillProgress = w->GetSkillProgress();
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
	if (sword_) {
		// ===== スキル「回転斬り」：剣を手から離して弧を描かせる =====
		// PlayerWeapon が持つスキル状態を Sword に橋渡しする。
		// IsSpinArcActive() を見ることで開始/終了のエッジを安全に扱う。
		if (weaponSkillActive && !sword_->IsSpinArcActive()) {
			sword_->BeginSpinArc();                       // 発動：手から切り離す
		}
		if (sword_->IsSpinArcActive()) {
			sword_->SetSpinArcProgress(weaponSkillProgress); // 進行度を反映
		}
		if (!weaponSkillActive && sword_->IsSpinArcActive()) {
			sword_->EndSpinArc();                         // 終了：手に戻す
		}

		// 通常攻撃のヒット区間（スキル中は Sword 側が常時ONにする）
		sword_->SetHitEnabled(weaponHitActive);
		sword_->Update();
	}

	// Player 本体コライダー更新
	isCollided_ = false;
	Sphere& sp = multiCollider_->MutableSphere(0);
	sp.center = colliderTranslate_;
	sp.radius = sphereRadius_;

	/*poweder->Update();*/
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

	if (isDead_) return; // 死亡中は被弾無視

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

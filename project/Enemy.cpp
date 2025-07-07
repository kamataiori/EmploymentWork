#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include "EnemyState_Idle.h"
#include "EnemyState_Dash.h"
#include "Player.h"
#include <EnemyState_Attack2.h>
#include <EnemyState_Attack1.h>

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");
	ModelManager::GetInstance()->LoadModel("matest.obj");

	object3d_->SetModel("matest.obj");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);

	ChangeState(std::make_unique<EnemyState_Idle>());

	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	//SetRotation(object3d_->GetRotate());
	//SetScale(object3d_->GetScale());
	sphere.radius = 1.5f;

	SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
}

void Enemy::Update()
{
	// ImGuiデバッグ表示
	ImGui::Begin("Enemy Debug");

	if (currentState_) {
		// 現在のステート名を表示
		ImGui::Text("Current State: %s", currentState_->GetName());
	}
	else {
		ImGui::Text("Current State: None");
	}

	ImGui::End();

	// プレイヤーの方向を向く
	if (player_) {
		Vector3 toPlayer = player_->GetTransform().translate - this->GetTransform().translate;

		// Y軸方向だけで角度を計算（上下方向は無視）
		float angleY = std::atan2(toPlayer.x, toPlayer.z);

		transform.rotate.y = angleY;
	}


	// ステート更新処理
	if (currentState_) {
		currentState_->Update(this);
	}

	// 範囲攻撃オブジェクトの更新
	for (auto& a : areaAttacks_) {
		a->Update();
	}
	areaAttacks_.remove_if([](const std::unique_ptr<EnemyAreaAttack>& a) {
		return a->IsDead();
		});

	// 通常攻撃オブジェクトの更新
	for (auto& b : bullets_) {
		b->Update();
	}
	bullets_.remove_if([](const auto& b) { return b->IsDead(); });


	//ImGui::Begin("Enemy Transform");

	//// Translate (位置)
	//Vector3 position = object3d_->GetTranslate();
	//if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
	//	object3d_->SetTranslate(position);
	//}

	//// Rotate (回転)
	//Vector3 rotation = object3d_->GetRotate();
	//if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
	//	object3d_->SetRotate(rotation);
	//}

	//// Scale (スケール)
	//Vector3 scale = object3d_->GetScale();
	//if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
	//	object3d_->SetScale(scale);
	//}

	//ImGui::End();

	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

	SetPosition(object3d_->GetTranslate());
	sphere.color = static_cast<int>(Color::WHITE);
}

void Enemy::Draw()
{

	for (auto& a : areaAttacks_) {
		a->Draw();
	}

	for (auto& b : bullets_) {
		b->Draw();
	}

	// SphereCollider の描画
	SphereCollider::Draw();
}

void Enemy::DrawModel()
{
	object3d_->Draw();
}

void Enemy::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);
}

void Enemy::ChangeState(std::unique_ptr<EnemyState> State)
{
	// 同じ状態名ならスキップ
	if (currentState_ && std::string(currentState_->GetName()) == State->GetName()) {
		return;
	}

	// 状態を切り替え
	currentState_ = std::move(State);
	currentState_->Enter(this);
}

void Enemy::ChangeToRandomState() {
	// 次の状態はIdle（待機）にするかどうか
	static bool insertIdleNext = true;

	if (insertIdleNext) {
		// 一度Idle（待機）を挟む
		insertIdleNext = false;
		previousStateName_ = "Idle";
		ChangeState(std::make_unique<EnemyState_Idle>());
		return;
	}

	// 各行動と重み（確率）のペア
	struct WeightedState {
		std::string name; // ステート名
		float weight;     // 重み（選ばれやすさ）
		std::function<std::unique_ptr<EnemyState>()> factory; // ステートを生成する関数
	};

	// ステート候補を定義
	std::vector<WeightedState> candidates = {
		{"Dash", dashWeight_, []() { return std::make_unique<EnemyState_Dash>(); }},
		{"Attack1", attack1Weight_, []() { return std::make_unique<EnemyState_Attack1>(); }},
		{"Attack2", attack2Weight_, []() { return std::make_unique<EnemyState_Attack2>(); }}
	};

	// 前回と同じステート名を除外（連続行動を避ける）
	std::erase_if(candidates, [this](const WeightedState& s) {
		return s.name == previousStateName_;
		});

	// すべて除外された場合は復元（行動できなくなるのを防ぐ）
	if (candidates.empty()) {
		candidates = {
			{"Dash", dashWeight_, []() { return std::make_unique<EnemyState_Dash>(); }},
			{"Attack1", attack1Weight_, []() { return std::make_unique<EnemyState_Attack1>(); }},
			{"Attack2", attack2Weight_, []() { return std::make_unique<EnemyState_Attack2>(); }}
		};
	}

	// 重みに基づくランダム選択
	float totalWeight = 0.0f;
	for (const auto& c : candidates) {
		totalWeight += c.weight;
	}

	// 0〜totalWeight の範囲でランダムに選ぶ
	float rnd = static_cast<float>(rand()) / RAND_MAX * totalWeight;
	float accum = 0.0f;

	// ランダム値がどの範囲に入るかでステート決定
	for (const auto& c : candidates) {
		accum += c.weight;
		if (rnd <= accum) {
			previousStateName_ = c.name;    // 今回のステート名を記録
			insertIdleNext = true;          // 次回はIdleを挟むように設定
			ChangeState(c.factory());       // ステート切り替え
			return;
		}
	}
}

void Enemy::AddAreaAttack(std::unique_ptr<EnemyAreaAttack> attack)
{
	areaAttacks_.push_back(std::move(attack));
}

void Enemy::AddBullet(std::unique_ptr<EnemyAttackBullet> bullet)
{
	bullets_.push_back(std::move(bullet));
}

Vector3 Enemy::GetPlayerPos() const
{
	if (player_) {
		return player_->GetTransform().translate;
	}
	return { 0, 0, 0 }; // 参照が無ければ原点
}

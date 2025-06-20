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

	object3d_->SetModel("uvChecker.gltf");

	// モデルにSRTを設定
	object3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	object3d_->SetRotate({ 0.0f, 3.14f, 0.0f });
	object3d_->SetTranslate({ 2.0f, 0.0f, 0.0f });

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
	//ImGui::Begin("Enemy Debug");

	//if (currentState_) {
	//	// 現在のステート名を表示
	//	ImGui::Text("Current State: %s", currentState_->GetName());
	//}
	//else {
	//	ImGui::Text("Current State: None");
	//}

	//ImGui::End();

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

	SetPosition(object3d_->GetTranslate());
	object3d_->SetTranslate(transform.translate);
	object3d_->Update();
	//SetScale(object3d_->GetScale());
	sphere.color = static_cast<int>(Color::WHITE);
}

void Enemy::Draw()
{
	object3d_->Draw();

	for (auto& a : areaAttacks_) {
		a->Draw();
	}

	for (auto& b : bullets_) {
		b->Draw();
	}

	// SphereCollider の描画
	SphereCollider::Draw();
}

void Enemy::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);

	//for (auto& attack : areaAttacks_) {
	//	attack->OnCollision(); // 各範囲攻撃オブジェクトに通知
	//}
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
	// すべてのステート候補を登録
	std::vector<std::pair<std::string, std::function<std::unique_ptr<EnemyState>()>>> stateFactories = {
		{"Idle", []() { return std::make_unique<EnemyState_Idle>(); }},
		{"Dash", []() { return std::make_unique<EnemyState_Dash>(); }},
		{"Attack1", []() { return std::make_unique<EnemyState_Attack1>(); }},
		{"Attack2", []() { return std::make_unique<EnemyState_Attack2>(); }},
	};

	// 前回と同じ名前を除外
	std::erase_if(stateFactories, [this](const auto& pair) {
		return pair.first == previousStateName_;
		});

	// 候補がすべて除外されてしまった場合、前回と同じでも許可（無限ループ回避）
	if (stateFactories.empty()) {
		stateFactories = {
			{"Idle", []() { return std::make_unique<EnemyState_Idle>(); }},
			{"Dash", []() { return std::make_unique<EnemyState_Dash>(); }},
			{"Attack1", []() { return std::make_unique<EnemyState_Attack1>(); }},
			{"Attack2", []() { return std::make_unique<EnemyState_Attack2>(); }},
		};
	}

	// ランダムに選択
	int index = rand() % stateFactories.size();
	std::unique_ptr<EnemyState> nextState = stateFactories[index].second();

	// 状態名を記録し、ステートを切り替え
	previousStateName_ = stateFactories[index].first;
	ChangeState(std::move(nextState));
}

void Enemy::Move(const Vector3& velocity) {
	transform.translate += velocity;
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

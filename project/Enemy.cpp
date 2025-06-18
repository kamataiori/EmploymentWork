#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include "EnemyState_Idle.h"
#include "EnemyState_Dash.h"
#include "Player.h"

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
	ImGui::Begin("Enemy Debug");

	if (currentState_) {
		// 現在のステート名を表示
		ImGui::Text("Current State: %s", currentState_->GetName());
	}
	else {
		ImGui::Text("Current State: None");
	}

	ImGui::End();

	// ステート更新処理
	if (currentState_) {
		currentState_->Update(this);
	}

	ImGui::Begin("Enemy Transform");

	// Translate (位置)
	Vector3 position = object3d_->GetTranslate();
	if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
		object3d_->SetTranslate(position);
	}

	// Rotate (回転)
	Vector3 rotation = object3d_->GetRotate();
	if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
		object3d_->SetRotate(rotation);
	}

	// Scale (スケール)
	Vector3 scale = object3d_->GetScale();
	if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
		object3d_->SetScale(scale);
	}

	ImGui::End();

	SetPosition(object3d_->GetTranslate());
	object3d_->SetTranslate(transform.translate);
	object3d_->Update();
	//SetScale(object3d_->GetScale());
	sphere.color = static_cast<int>(Color::WHITE);
}

void Enemy::Draw()
{
	object3d_->Draw();
	// SphereCollider の描画
	SphereCollider::Draw();
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

void Enemy::Move(const Vector3& velocity) {
	transform.translate += velocity;
}

Vector3 Enemy::GetPlayerPos() const
{
	if (player_) {
		return player_->GetTransform().translate;
	}
	return { 0, 0, 0 }; // 参照が無ければ原点
}

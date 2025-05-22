#include "Player.h"
#include <CollisionTypeIdDef.h>

void Player::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");

	object3d_->SetModel("uvChecker.gltf");

	transform_.translate = { -2.0f, 0.0f, 0.0f };
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 3.14f, 0.0f };

	// モデルにSRTを設定
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);

	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	sphere.radius = 2.0f;
	//SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// レティクルスプライト初期化
	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Initialize("Resources/aiming.png");

	// 3Dレティクルの初期化
	//reticle3D_ = std::make_unique<Object3d>(baseScene_);
	//reticle3D_->Initialize();
	//reticle3D_->SetModel("uvChecker.gltf"); // 色付きや小さめモデルにしてもOK
	//reticleTransform_.scale = { 1.0f, 1.0f, 1.0f }; // 小さめ表示

}

void Player::Update()
{
	// 入力方向ベクトル
	Vector3 input{};
	if (Input::GetInstance()->PushKey(DIK_W)) { input.z += 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_S)) { input.z -= 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_A)) { input.x -= 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_D)) { input.x += 4.0f; }

	// 正規化して移動方向を一定に
	if (input.x != 0.0f || input.z != 0.0f) {
		input = Normalize(input);
	}

	// 移動速度（最大速度）
	float moveSpeed = 2.0f;

	// イージング係数
	float ease = 0.25f;

	// velocity_ を滑らかに補間
	velocity_ = Lerp(velocity_, input * moveSpeed, ease);

	// 移動反映
	transform_.translate += velocity_;

	// レティクルの位置を中央に設定
	float sw = static_cast<float>(1280);
	float sh = static_cast<float>(720);
	float spriteSize = 32.0f;

	reticleSprite_->SetPosition({ (sw - spriteSize) / 2.0f, (sh - spriteSize) / 2.0f });
	reticleSprite_->Update();

	// まず描画と同じようにスケール・回転・平行移動を反映
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();  // 行列がここで更新される


	// キー押下時に発射
	if (Input::GetInstance()->TriggerMouseLeft() && canShoot_ && bulletStock_ > 0) {
		Matrix4x4 rotMat = MakeRotateMatrix(transform_.rotate);
		Vector3 forward = TransformNormal({ 0, 0, -1 }, rotMat);
		bullets_.push_back(std::make_unique<PlayerBullet>(
			transform_.translate, forward, baseScene_, camera_));
		bulletStock_--;
		canShoot_ = false;
	}

	if (!Input::GetInstance()->PushMouseLeft()) {
		canShoot_ = true;
	}

	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		bulletStock_ = 10; // リロード
	}



	// 弾の更新
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		}
		else {
			++it;
		}
	}
	

	// デバッグと描画更新
	Debug();
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
	SetPosition(object3d_->GetTranslate());
}

void Player::Draw()
{
	//reticle3D_->Draw();

	object3d_->Draw();
	// SphereCollider の描画
	SphereCollider::Draw();
}

void Player::Draw2D()
{
	reticleSprite_->Draw();
}

void Player::Debug()
{
#ifdef _DEBUG

	ImGui::Begin("Player Transform");

	// Translate
	Vector3 position = object3d_->GetTranslate();
	if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
		object3d_->SetTranslate(position);
	}

	// Rotate
	Vector3 rotation = object3d_->GetRotate();
	if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
		object3d_->SetRotate(rotation);
	}

	// Scale
	Vector3 scale = object3d_->GetScale();
	if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
		object3d_->SetScale(scale);
	}

	ImGui::End();

#endif _DEBUG
}

void Player::DrawBullets()
{
	for (const auto& b : bullets_) {
		b->Draw();
	}
}

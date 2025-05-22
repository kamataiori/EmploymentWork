#include "Player.h"
#include <CollisionTypeIdDef.h>

// プレイヤー初期化
void Player::Initialize()
{
	// モデル読み込みと設定
	InitializeModel();

	// 座標・回転・スケール初期化
	InitializeTransform();

	// 当たり判定の初期化
	InitializeCollider();

	// レティクルスプライト初期化
	InitializeUI();
}

// 毎フレーム更新処理
void Player::Update()
{
	// 入力による移動処理
	HandleMovement();

	// レティクルUI更新
	UpdateUI();

	// マウスクリックでの弾発射
	HandleShooting();

	// 弾の更新と消滅処理
	UpdateBullets();

	// Object3dへTransform反映
	UpdateTransform();

	// デバッグUI表示
	Debug();
}

// プレイヤー本体の描画
void Player::Draw()
{
	// モデル描画
	object3d_->Draw();

	// コライダー描画
	SphereCollider::Draw();
}

// レティクルなどの2D描画
void Player::Draw2D()
{
	reticleSprite_->Draw();
}

// ImGuiでデバッグ表示
void Player::Debug()
{
#ifdef _DEBUG
	ImGui::Begin("Player Transform");

	Vector3 position = object3d_->GetTranslate();
	if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
		object3d_->SetTranslate(position);
	}

	Vector3 rotation = object3d_->GetRotate();
	if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f)) {
		object3d_->SetRotate(rotation);
	}

	Vector3 scale = object3d_->GetScale();
	if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
		object3d_->SetScale(scale);
	}

	ImGui::Text("Bullet Stock: %d", bulletStock_);
	ImGui::End();
#endif
}

// 弾の描画
void Player::DrawBullets()
{
	for (const auto& b : bullets_) {
		b->Draw();
	}
}

// モデルを読み込んでObject3dに設定
void Player::InitializeModel()
{
	object3d_->Initialize();
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	object3d_->SetModel("uvChecker.gltf");
}

// 初期Transformの設定
void Player::InitializeTransform()
{
	transform_.translate = { -2.0f, 0.0f, 0.0f };
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 3.14f, 0.0f };
	UpdateTransform();
}

// コライダーを設定
void Player::InitializeCollider()
{
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());
	sphere.radius = 2.0f;
}

// レティクルスプライト初期化
void Player::InitializeUI()
{
	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Initialize("Resources/aiming.png");
}

// レティクルスプライト位置の更新
void Player::UpdateUI()
{
	float sw = static_cast<float>(1280);
	float sh = static_cast<float>(720);
	float spriteSize = 32.0f;
	reticleSprite_->SetPosition({ (sw - spriteSize) / 2.0f, (sh - spriteSize) / 2.0f });
	reticleSprite_->Update();
}

// 移動入力処理
void Player::HandleMovement()
{
	Vector3 input{};
	if (Input::GetInstance()->PushKey(DIK_W)) { input.z += 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_S)) { input.z -= 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_A)) { input.x -= 4.0f; }
	if (Input::GetInstance()->PushKey(DIK_D)) { input.x += 4.0f; }

	if (input.x != 0.0f || input.z != 0.0f) {
		input = Normalize(input);
	}

	float moveSpeed = 2.0f;
	float ease = 0.25f;
	velocity_ = Lerp(velocity_, input * moveSpeed, ease);
	transform_.translate += velocity_;
}

// マウス左クリックで弾を発射する処理
void Player::HandleShooting()
{
	if (Input::GetInstance()->TriggerMouseLeft() && canShoot_ && bulletStock_ > 0) {
		Matrix4x4 rotMat = MakeRotateMatrix(transform_.rotate);
		Vector3 forward = TransformNormal({ 0, 0, -1 }, rotMat);
		bullets_.push_back(std::make_unique<PlayerBullet>(transform_.translate, forward, baseScene_, camera_));
		bulletStock_--;
		canShoot_ = false;
	}

	// ボタンを離したら再発射可能に
	if (!Input::GetInstance()->PushMouseLeft()) {
		canShoot_ = true;
	}

	// Rキーで弾数リロード
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		bulletStock_ = 10;
	}
}

// 弾の更新と寿命チェック
void Player::UpdateBullets()
{
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		}
		else {
			++it;
		}
	}
}

// TransformをObject3dへ反映
void Player::UpdateTransform()
{
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
	SetPosition(object3d_->GetTranslate());
}

Vector3 Player::TransformNormal(const Vector3& v, const Matrix4x4& m)
{
	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
	};
	return result;
}

Vector3 Player::Multiply(const Vector3& v, const Matrix4x4& m)
{
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	return result;
}

Matrix4x4 Player::Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result{};

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			result.m[y][x] =
				m1.m[y][0] * m2.m[0][x] +
				m1.m[y][1] * m2.m[1][x] +
				m1.m[y][2] * m2.m[2][x] +
				m1.m[y][3] * m2.m[3][x];
		}
	}

	return result;
}

Matrix4x4 Player::MakeRotateMatrix(const Vector3& rotate)
{
	Matrix4x4 resultX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 resultY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 resultZ = MakeRotateZMatrix(rotate.z);
	return Multiply(resultX, Multiply(resultY, resultZ));
}

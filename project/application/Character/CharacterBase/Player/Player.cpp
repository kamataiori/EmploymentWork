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

	// 3Dレティクルの初期化
	ModelManager::GetInstance()->LoadModel("reticle3D.obj");
	test_ = std::make_unique<Object3d>(baseScene_);
	test_->Initialize();
	test_->SetModel("reticle3D.obj");
	test_->SetCamera(camera_);
	testTransform_.translate = { 0.0f,0.0f,100.0f };
	test_->SetTranslate(testTransform_.translate);
	/*reticle3D_ = std::make_unique<Object3d>(baseScene_);
	reticle3D_->Initialize();
	reticle3D_->SetModel("reticle3D.obj"); 
	reticle3D_->SetCamera(camera_);
	reticleTransform_.scale = { 1.0f,1.0f,1.0f };
	reticle3D_->SetScale(reticleTransform_.scale);*/
	//reticle3D_->SetTranslate(reticleTransform_.translate);

}

// 毎フレーム更新処理
void Player::Update()
{
	// 入力による移動処理（translateを更新）
	HandleMovement();

	// 自機TransformをObject3dに反映（発射に備えて位置を確定）
	UpdateTransform();

	// 3Dレティクルのワールド固定位置に設置（自機位置の後に！）
	/*reticleTransform_.translate = { transform_.translate.x,0.0f,transform_.translate.z + 30.0f };
	reticle3D_->SetScale(reticleTransform_.scale);
	reticle3D_->SetTranslate(reticleTransform_.translate);
	reticle3D_->Update();*/

	// 発射処理（transformとreticleの位置が確定後）
	HandleShooting();

	// 弾の更新
	UpdateBullets();

	// UI更新（2Dレティクルなど）
	UpdateUI();

	// デバッグ表示
	Debug();

	test_->Update();
	//test_->SetTranslate(testTransform_.translate);

	Vector3 world = testTransform_.translate;
	/*Vector3 testWorldPos = {
		world.m[3][0],
		world.m[3][1],
		world.m[3][2]
	};*/

	ImGui::Begin("Test Control");
	Vector3 pos = test_->GetTranslate();
	if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
		test_->SetTranslate(pos);
	}
	ImGui::End();

}

// プレイヤー本体の描画
void Player::Draw()
{
	// モデル描画
	object3d_->Draw();

	//reticle3D_->Draw();
	test_->Draw();

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
void Player::UpdateUI() {
	// スクリーンサイズ
	float sw = 1280.0f;
	float sh = 720.0f;
	float spriteSize = 32.0f;

	Vector2 screenPos = {
		(sw - spriteSize) / 2.0f,
		(sh - spriteSize) / 2.0f
	};
	reticleSprite_->SetPosition(screenPos);
	reticleSprite_->Update();

}


// 移動入力処理
void Player::HandleMovement()
{
	Vector3 input{};
	if (Input::GetInstance()->PushKey(DIK_W)) { input.z += 1.0f; }
	if (Input::GetInstance()->PushKey(DIK_S)) { input.z -= 1.0f; }
	if (Input::GetInstance()->PushKey(DIK_A)) { input.x -= 1.0f; }
	if (Input::GetInstance()->PushKey(DIK_D)) { input.x += 1.0f; }

	if (input.x != 0.0f || input.z != 0.0f) {
		input = Normalize(input);
	}

	float moveSpeed = 2.0f;
	velocity_ = input * moveSpeed;

	transform_.translate += velocity_;
}


// マウス左クリックで弾を発射する処理
void Player::HandleShooting() {
	if (Input::GetInstance()->TriggerMouseLeft() && canShoot_ && bulletStock_ > 0) {
		// プレイヤーのワールド座標
		Matrix4x4 world = object3d_->GetWorldMatrix();
		Vector3 playerWorldPos = {
			world.m[3][0],
			world.m[3][1],
			world.m[3][2]
		};

		//playerWorldPos = playerWorldPos - camera_->GetTranslate();

		// スクリーンサイズ（ハードコードまたは定数で）
		const float screenWidth = 1280.0f;
		const float screenHeight = 720.0f;

		// 2Dレティクルのスプライト左上の位置
		Vector2 reticlePos = reticleSprite_->GetPosition();

		// スプライトサイズの中心分だけオフセットして中央に補正（64x64前提）
		Vector2 centerPos = {
			reticlePos.x + 32.0f,
			reticlePos.y + 32.0f
		};

		// スクリーン → NDC 変換（Y軸反転）
		Vector3 ndc;
		ndc.x = (centerPos.x / screenWidth) * 2.0f - 1.0f;
		ndc.y = 1.0f - (centerPos.y / screenHeight) * 2.0f;
		ndc.z = 1.0f; // 遠方を想定

		// ビュー×プロジェクションの逆行列で NDC → ワールド座標
		Matrix4x4 viewProj = Multiply(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
		Matrix4x4 invViewProj = Inverse(viewProj);
		Vector3 worldFar = TransformCoord(ndc, invViewProj); // NDC → ワールド空間座標

		worldFar.x = worldFar.x + camera_->GetWorldMatrix().m[3][0];
		worldFar.y = worldFar.y + camera_->GetWorldMatrix().m[3][1];
		worldFar.z = 50.0f;

		// 発射方向ベクトル（カメラ中央方向）
		Vector3 direction = Normalize(worldFar - playerWorldPos);

		// 弾を発射
		bullets_.push_back(std::make_unique<PlayerBullet>(
			playerWorldPos, direction, baseScene_, camera_));

		bulletStock_--;
		canShoot_ = false;
	}

	if (!Input::GetInstance()->PushMouseLeft()) {
		canShoot_ = true;
	}

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

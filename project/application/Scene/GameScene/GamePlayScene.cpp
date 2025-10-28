#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include <algorithm>
#include <cmath>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


// ---------- 小ユーティリティ ----------
namespace {
	inline float EaseInOutCubic01(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return (t < 0.5f) ? 4.0f * t * t * t
			: 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
	}
	inline Vector2 LerpVec2f(const Vector2& a, const Vector2& b, float t) {
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
	}
}

namespace {
	inline Vector3 LookAtEuler(const Vector3& eye, const Vector3& target) {
		Vector3 d{ target.x - eye.x, target.y - eye.y, target.z - eye.z };
		float len = std::max(1e-6f, std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z));
		d.x /= len; d.y /= len; d.z /= len;
		float yaw = std::atan2(d.x, d.z);
		float pitch = -std::asin(std::clamp(d.y, -1.0f, 1.0f)); // 上向きがマイナス前提
		return { pitch, yaw, 0.0f };
	}
	inline float Smoothstep(float t) { t = std::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }
}



void GamePlayScene::Initialize()
{
	// ライト
	// Lightクラスのデータを初期化
	BaseScene::GetLight()->Initialize();
	BaseScene::GetLight()->GetCameraLight();
	BaseScene::GetLight()->GetDirectionalLight();
	BaseScene::GetLight()->SetDirectionalLightIntensity({ 1.0f });
	BaseScene::GetLight()->SetDirectionalLightColor({ 1.0f,1.0f,1.0f,1.0f });

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("Warrior.gltf");
	ModelManager::GetInstance()->LoadModel("Rogue.gltf");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("stage.obj");
	ModelManager::GetInstance()->LoadModel("skydome.obj");

	// === カメラ生成 ===
	enemyIntroCam_ = std::make_unique<Camera>();

	currentCamera_ = enemyIntroCam_.get();  // 初期カメラを設定

	// 任意の汎用カメラ
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	// ===== 主要アクター =====

	player_ = std::make_unique<Player>(this);
	followCamera = std::make_unique<FollowCamera>(nullptr, 35.0f, 7.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize(followCamera.get());
	followCamera->SetTarget(player_->GetCurrentCharacter());

	enemy_ = std::make_unique<Enemy>(this);
	enemy_->Initialize();

	// プレイヤー参照を敵へ“先に”渡す
	enemy_->SetPlayer(player_.get());

	// その後で弾専用に切替
	enemy_->ChangeToBulletOnlyState();

	// ===== 環境 =====
	skybox = std::make_unique<SkyBox>();
	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,-1.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	// ===== ステージ（レベルデータ） =====
	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage"); // Resources/Json/stage.json を読み込む

	// ===== カメラの初期選択（重要！）=====
	// シャッター中は followCamera で全体を描画する
	currentCamera_ = followCamera.get();
	ApplyCurrentCameraToAll();   // ← 全描画対象に現在のカメラを再バインド

	// ===== 当たり判定管理 =====
	collisionMAnager_ = std::make_unique<CollisionManager>();

	// ===== デバッグウィンドウ =====
	AddRightDockWindow(kWindowName_MonsterControl);

	// ===== 逆シャッターのセットアップ =====
	const float screenW = 1280.0f;
	const float screenH = 720.0f;

	shutterTop_ = std::make_unique<Sprite>();
	shutterBottom_ = std::make_unique<Sprite>();
	shutterTop_->Initialize("Resources/Black.png");
	shutterBottom_->Initialize("Resources/Black.png");

	// アンカー（Title の閉演出と対にする）
	shutterTop_->SetAnchorPoint({ 0.5f, 1.0f });  // 下辺基準
	shutterBottom_->SetAnchorPoint({ 0.5f, 0.0f }); // 上辺基準

	// 画面を覆えるサイズ（片側で半分＋少し重ね気味）
	shutterTop_->SetSize({ screenW, screenH * 0.55f });
	shutterBottom_->SetSize({ screenW, screenH * 0.55f });

	// 位置定義：Title と正反対
	shutterOpen_.topStart = { screenW * 0.5f, 0.0f };         // 画面上外（開き終わり）
	shutterOpen_.botStart = { screenW * 0.5f, screenH };      // 画面下外（開き終わり）
	shutterOpen_.topEnd = { screenW * 0.5f, screenH * 0.5f }; // 中央で閉（開始）
	shutterOpen_.botEnd = { screenW * 0.5f, screenH * 0.5f };

	// 初期配置：まずは中央でぴったり閉じている（真っ黒）
	shutterTop_->SetPosition(shutterOpen_.topEnd);
	shutterBottom_->SetPosition(shutterOpen_.botEnd);

	// シーン開始と同時に “中央で少しホールド → ゆっくり開く”
	BeginShutterOpen(/*duration=*/4.0f, /*holdAtCenter=*/0.8f);
	SwitchPhase(Phase::ShutterOpen);

	// ===== Ready / Go スプライト =====
	ready_ = std::make_unique<Sprite>();
	go_ = std::make_unique<Sprite>();
	ready_->Initialize("Resources/ready.png");
	go_->Initialize("Resources/go.png");
	ready_->SetAnchorPoint({ 0.5f, 0.5f });
	go_->SetAnchorPoint({ 0.5f, 0.5f });
	ready_->SetPosition({ screenW * 0.5f, screenH * 0.3f });
	go_->SetPosition({ screenW * 0.5f, screenH * 0.3f });
	readyAlpha_ = 0.0f;
	goAlpha_ = 0.0f;
}

void GamePlayScene::Finalize()
{

}

void GamePlayScene::Update()
{
	// 各3Dオブジェクトの更新
	stage_->Update();
	skybox->Update();
	ground->Update();
	sky->Update();
	player_->Update();
	enemy_->Update();

	// カメラの更新
	camera1->Update();
	followCamera->Update();


	// ===== 逆シャッター進行 =====
	const float dt = 1.0f / 60.0f;
	phaseTimer_ += dt;

	if (shutterOpen_.active) {

		// 開き始める前に少し“黒で間”をおく
		if (shutterOpen_.holdTimer < shutterOpen_.holdSec) {
			shutterOpen_.holdTimer += dt;

			// 常に中央位置（真っ黒のまま）
			shutterTop_->SetPosition(shutterOpen_.topEnd);
			shutterBottom_->SetPosition(shutterOpen_.botEnd);
		}
		else {
			// 進捗を増やして外側へ移動（中央→画面外）
			shutterOpen_.t = std::min(1.0f,
				shutterOpen_.t + dt / (std::max)(0.001f, shutterOpen_.duration));
			float e = EaseInOutCubic01(shutterOpen_.t);
			Vector2 topPos = LerpVec2f(shutterOpen_.topEnd, shutterOpen_.topStart, e);
			Vector2 botPos = LerpVec2f(shutterOpen_.botEnd, shutterOpen_.botStart, e);


			shutterTop_->SetPosition(topPos);
			shutterBottom_->SetPosition(botPos);

			if (shutterOpen_.t >= 1.0f) {
				// 完全に画面外へ出たら演出終了（非アクティブ）
				shutterOpen_.active = false;
			}
		}

		// スプライト内部更新
		shutterTop_->Update();
		shutterBottom_->Update();
	}

	// ===== フェーズ別処理 =====
	switch (phase_) {
	case Phase::ShutterOpen: {
		// 開け終わったらフライオーバー
		if (!shutterOpen_.active) {
			// ★ ここで落下演出を必ず開始させる（これが無いと即 ReadyGo へ行く）
			enemy_->BeginIntroFall(
				/*start*/{ 0.0f, 18.0f, 0.0f },
				/*end  */{ 0.0f,  0.0f, 0.0f },
				/*durationSec*/ 2.2f,   // 好きな尺でOK（ゆっくりなら 2.0〜2.5）
				/*bounce    */ 0.12f
			);

			currentCamera_ = enemyIntroCam_.get();
			ApplyCurrentCameraToAll();
			SwitchPhase(Phase::EnemyIntro);
		}
	} break;

	case Phase::EnemyIntro: { // ← ブロック必須
		enemy_->UpdateBefore();

		// 低い位置から敵を見上げ続ける
		Vector3 eye = eicam_.pos;

		// 敵の現在位置（無ければ Enemy に Getter を1行追加：GetWorldPosition()）
		Vector3 tgt = enemy_->GetTransform().translate;
		tgt.y += 0.6f; // 目線より少し下を狙って見上げ感強調（好みで0.3〜1.0）

		// --- ドリーイン（踏まれる直前まで寄る）任意：1.0秒で80%寄る ---
		static float introT = 0.0f;
		introT = std::min(1.0f, introT + dt / 1.0f);
		float k = Smoothstep(introT);
		eye.z += (-0.35f) * k;   // 手前へ寄る（値を -0.2〜-0.6 で調整）

		// --- 着地フレームで stomp エフェクトを起動 ---
		if (enemy_->ConsumeJustLanded()) {
			stompShakeTime_ = stompShakeDur_;   // シェイク開始
		}

		// --- シェイク適用（指数減衰＋縦強め） ---
		float fov = stompFovBase_;
		if (stompShakeTime_ > 0.0f) {
			float n = stompShakeTime_ / stompShakeDur_;
			float decay = n * n; // 二乗減衰
			// 簡易ノイズ（sin/cos位相ずらし）
			float t = (stompShakeDur_ - stompShakeTime_) * 60.0f;
			Vector3 shake{
				(std::sin(t * 0.37f)) * stompShakeAmp_ * 0.4f * decay,
				(std::sin(t * 0.53f)) * stompShakeAmp_ * 1.0f * decay, // 縦を強め
				(std::cos(t * 0.29f)) * stompShakeAmp_ * 0.3f * decay
			};
			eye.x += shake.x;
			eye.y += shake.y;
			eye.z += shake.z;

			fov += stompFovKick_ * decay; // FOVキックで圧を出す
			stompShakeTime_ -= dt;
			if (stompShakeTime_ < 0.0f) stompShakeTime_ = 0.0f;
		}

		// 回転を敵へ向ける
		Vector3 rot = LookAtEuler(eye, tgt);

		enemyIntroCam_->SetTranslate(eye);
		enemyIntroCam_->SetRotate(rot);
		enemyIntroCam_->SetFovY(fov);
		enemyIntroCam_->Update();

		// 落下＋ポーズが完全終了したら次へ
		if (!enemy_->IsIntroFalling()) {
			// 後処理
			introT = 0.0f;
			currentCamera_ = followCamera.get();
			ApplyCurrentCameraToAll();
			enemy_->SetScale({ 5.0f, 5.0f, 5.0f });
			enemy_->SetRotate({ 0.0f, 3.14f, 0.0f });
			enemy_->SetPlayer(player_.get());
			enemy_->Update();
			SwitchPhase(Phase::ReadyGo);
		}
	} break;

	case Phase::ReadyGo: { // ← ブロック必須
		// Ready をフェードアウト → Go をフェードアウト
		const float readyFadeTime = 1.8f;   // Ready表示からゆっくり消えるまで（旧0.9f）
		const float goFadeTime = 1.6f;   // Go表示からゆっくり消えるまで（旧0.8f）

		if (phaseTimer_ < readyFadeTime) {
			// Ready フェード中
			float t = phaseTimer_ / readyFadeTime;
			readyAlpha_ = std::max(0.0f, 1.0f - t);  // 徐々に消える
			goAlpha_ = 0.0f;
		}
		else {
			// Go フェード中
			float t = (phaseTimer_ - readyFadeTime) / goFadeTime;
			goAlpha_ = std::max(0.0f, 1.0f - t);
			readyAlpha_ = 0.0f;
		}

		// 終了判定（Ready + Go の合計時間を過ぎたらBattleへ）
		if (phaseTimer_ > readyFadeTime + goFadeTime) {
			SwitchPhase(Phase::Battle);
		}
	} break;

	case Phase::Battle: {
		collisionMAnager_->RegisterCollider(player_->Get());
		collisionMAnager_->RegisterCollider(enemy_.get());
		/*if (player_->GetBullet()) {
			auto bullet = player_->GetBullet();
			collisionMAnager_->RegisterCollider(bullet);
		}*/
		/*for (const auto& areaAttack : enemy_->GetAreaAttacks()) {
			collisionMAnager_->RegisterCollider(areaAttack.get());
		}*/
		for (const auto& bulletAttack : enemy_->GetAttackBulets()) {
			collisionMAnager_->RegisterCollider(bulletAttack.get());
		}


		// 衝突判定と応答
		CheckAllColisions();
		// 以降は通常更新
	} break;
	}

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}



	//Debug();

	if (Input::GetInstance()->TriggerKey(DIK_T)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	if (Input::GetInstance()->TriggerKey(DIK_U)) {
		// シーン切り替え
		SceneManager::GetInstance()->ChangeScene("Unity");
	}

}

void GamePlayScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の背景描画
	// ================================================



	// ================================================
	// ここまでSprite個々の背景描画
	// ================================================
}

void GamePlayScene::Draw()
{

	//skybox->Draw();

	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	sky->Draw();
	ground->Draw();

	switch (phase_) {
	case Phase::ShutterOpen:
		// プレイヤのみ描画（敵は隠す）
		player_->Draw();
		// enemy_->Draw(); // 非表示
		break;

	case Phase::EnemyIntro:
		// 敵のみ描画（プレイヤは隠す）
		// player_->Draw(); // 非表示
		enemy_->Draw();
		break;

	case Phase::ReadyGo:
	case Phase::Battle:
		// 両方描画
		player_->Draw();
		enemy_->Draw();
		break;
	}


	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	switch (phase_) {
	case Phase::ShutterOpen:
		player_->SkinningDraw();
		// enemy_->DrawModel(); // 非表示
		break;

	case Phase::EnemyIntro:
		// player_->SkinningDraw(); // 非表示
		enemy_->DrawModel();
		break;

	case Phase::ReadyGo:
	case Phase::Battle:
		player_->SkinningDraw();
		enemy_->DrawModel();
		break;
	}


	// ================================================
	// ここまでアニメーションオブジェクトの個々の描画
	// ================================================

	// ================================================
	// ここからDrawLine個々の描画
	// ================================================



	// ================================================
	// ここまでDrawLine個々の描画
	// ================================================
}

void GamePlayScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================

	// === 画面最前面で逆シャッターを描く ===
	if (shutterOpen_.active) {
		shutterTop_->Draw();
		shutterBottom_->Draw();
	}

	// Ready / Go
	if (phase_ == Phase::ReadyGo || phase_ == Phase::Battle) {
		if (readyAlpha_ > 0.0f) {
			ready_->SetColor({ 1,1,1,readyAlpha_ });
			ready_->Update();
			ready_->Draw();
		}
		if (goAlpha_ > 0.0f) {
			go_->SetColor({ 1,1,1,goAlpha_ });
			go_->Update();
			go_->Draw();
		}
	}

	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	player_->ParticlDraw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void GamePlayScene::Debug()
{
#ifdef _DEBUG

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など

	if (ImGui::Begin("Intro Cameras")) {

		//// =============================
		//// フライオーバーカメラ設定
		//// =============================
		//if (ImGui::CollapsingHeader("Flyover Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		//	ImGui::Text("ステージをぐるっと見せるカメラ");

		//	ImGui::DragFloat3("Center", &fly_.center.x, 0.1f);
		//	ImGui::DragFloat("Radius", &fly_.radius, 0.1f, 1.0f, 200.0f);
		//	ImGui::DragFloat("Height", &fly_.height, 0.05f, -10.0f, 50.0f);
		//	ImGui::DragFloat("Speed(rad/s)", &fly_.speed, 0.01f, 0.0f, 4.0f);
		//	ImGui::DragFloat("FovY(deg)", &fly_.fovY, 0.001f);

		//	if (ImGui::Button("Reset Flyover")) {
		//		fly_.center = { 0,0,0 };
		//		fly_.radius = 35.0f;
		//		fly_.height = 12.0f;
		//		fly_.speed = 0.4f;
		//		fly_.fovY = 45.0f * 3.1415926f / 180.0f;
		//	}
		//}

		ImGui::Separator();

		// =============================
		// 敵登場カメラ設定
		// =============================
		if (ImGui::CollapsingHeader("Enemy Intro Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("敵が落下してくる時のカメラ（下から撮る）");

			ImGui::DragFloat3("Pos", &eicam_.pos.x, 0.05f);
			ImGui::DragFloat3("Rot(rad)", &eicam_.rot.x, 0.02f); // 角度はラジアン
			ImGui::DragFloat("FovY(rad)", &eicam_.fovY, 0.001f);

			if (ImGui::Button("Apply To Camera")) {
				enemyIntroCam_->SetTranslate(eicam_.pos);
				enemyIntroCam_->SetRotate(eicam_.rot);
				enemyIntroCam_->SetFovY(eicam_.fovY);
				enemyIntroCam_->Update();
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset IntroCam")) {
				eicam_.pos = { 0.0f, 1.2f, -6.5f };
				eicam_.rot = { -0.17f, 0.0f, 0.0f }; // -10°ほど上向き
				eicam_.fovY = 50.0f * 3.1415926f / 180.0f;
			}
		}

		ImGui::Separator();

		// =============================
		// 現在フェーズ表示
		// =============================
		const char* phaseNames[] = {
			"ShutterOpen", "EnemyIntro", "ReadyGo", "Battle"
		};
		ImGui::Text("Current Phase: %s", phaseNames[(int)phase_]);
		ImGui::ProgressBar(std::clamp(phaseTimer_ / 3.0f, 0.0f, 1.0f), ImVec2(200, 16), "Phase Timer");

		ImGui::Separator();

		// デバッグ用：現在カメラ座標確認
		if (currentCamera_) {
			ImGui::Text("Active Camera Position");
			Vector3 pos = currentCamera_->GetTranslate();
			ImGui::Text("  x: %.2f  y: %.2f  z: %.2f", pos.x, pos.y, pos.z);
		}
	}

	ImGui::End();

#endif
}

void GamePlayScene::CheckAllColisions()
{
	collisionMAnager_->CheckAllCollisions();
}

void GamePlayScene::ApplyCurrentCameraToAll()
{
	if (!currentCamera_) return;
	skybox->SetCamera(currentCamera_);
	ground->SetCamera(currentCamera_);
	sky->SetCamera(currentCamera_);
	stage_->SetCamera(currentCamera_);
	DrawLine::GetInstance()->SetCamera(currentCamera_);

	// 敵やプレイヤの内部でカメラ参照しているならここで
	enemy_->SetCamera(currentCamera_);
	// player_ 側の描画がカメラを保持している設計なら同様に：
	//player_->SetCamera(currentCamera_);
}

void GamePlayScene::SwitchPhase(Phase next)
{
	phase_ = next;
	phaseTimer_ = 0.0f;
}

void GamePlayScene::BeginShutterOpen(float duration, float holdAtCenter)
{
	shutterOpen_.active = true;
	shutterOpen_.t = 0.0f;
	shutterOpen_.duration = (std::max)(0.03f, duration);     // マクロ衝突回避で (std::max)
	shutterOpen_.holdSec = (std::max)(0.0f, holdAtCenter); // 同上
	shutterOpen_.holdTimer = 0.0f;

	// 中央で塞いだ状態から開始（真っ黒）
	shutterTop_->SetPosition(shutterOpen_.topEnd);
	shutterBottom_->SetPosition(shutterOpen_.botEnd);
}

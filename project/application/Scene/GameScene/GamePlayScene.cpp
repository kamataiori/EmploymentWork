#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include "engine/TimeManager.h"

#include "EnemyDropBullet.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

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

	// 3Dカメラの初期化
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });

	cameraEffect_ = std::make_unique<CameraEffectController>();

	player_ = std::make_unique<Player>(this);
	enemy_ = std::make_unique<Enemy>(this);

	followCamera = std::make_unique<FollowCamera>(player_.get(), 30.0f, 8.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize();
	player_->SetCamera(followCamera.get());
	/*followCamera->SetTarget(player_->Get());*/

	enemy_->Initialize();
	enemy_->SetCamera(followCamera.get());
	enemy_->SetTargetTransform(&player_->GetTransform());
	enemy_->SetCamera(followCamera.get());

	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,0.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	skybox->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	sky->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage"); // Resources/Json/stage.json を読み込む
	stage_->SetCamera(followCamera.get());



	collisionManager_ = std::make_unique<CollisionManager>();

	ex = std::make_unique<Sprite>();
	ex->Initialize("Resources/exp.png");
	ex->SetPosition({ 0.0f,100.0f });

	uiManager_ = std::make_unique<UIManager>();

	auto pause = std::make_unique<PauseScreen>();
	pause->Initialize({ 1280.0f, 720.0f }, "TITLE");
	uiManager_->Add(std::move(pause));

}

void GamePlayScene::Finalize()
{

}

void GamePlayScene::Update()
{
	// =========================
    // UI 更新（ESC入力・ポーズ判定含む）
    // =========================
	uiManager_->Update();

	// =========================
	// ポーズ中ならゲーム更新しない
	// =========================
	if (uiManager_->IsModalActive()) {
		return;
	}

		// 各3Dオブジェクトの更新
	stage_->Update();
	skybox->Update();
	ground->Update();
	sky->Update();
	player_->Update();
	enemy_->Update();

	ex->Update();

	// カメラの更新
	camera1->Update();
	// ロックされていないときだけ followCamera を更新する
	if (!followCameraLocked_) {
		followCamera->Update();
	}

	// ========= ここからカメラ演出入力 =========
	Input* input = Input::GetInstance();

	using ShakeMode = CameraEffectController::ShakeMode;
	using ZoomParams = CameraEffectController::ZoomParams;
	using MoveParams = CameraEffectController::MoveParams;


	// O キー：撃破カメラテスト（回り込み）
	// 敵死亡 → 撃破カメラ開始
	bool enemyDeadNow = enemy_->IsDead();  // 今の状態

	if (!enemyWasDead_ && enemyDeadNow)
	{
		// カメラ追従を止める
		followCameraLocked_ = true;

		// 回り込みの中心はプレイヤー位置
		const Transform& playerTf = player_->GetTransform();
		Vector3 center = playerTf.translate;

		// 回り込み用パラメータを組み立てる
		CameraEffectController::OrbitParams orbit{};
		float angleRad = std::numbers::pi_v<float> / 1.5f;

		orbit
			.Center(center)                 // どこを中心に回り込むか
			.Angle(angleRad)                // 回り込む角度
			.Duration(0.6f)                 // 1秒かけて
			.Easing(Tween::Easing::EaseInExpo); // 急激に加速するカーブ

		// これ1行で「回り込み＋プレイヤー注視」まで全部やってくれる
		cameraEffect_->StartOrbitMove(
			followCamera.get(),
			orbit
		);

		// ここでスローモーション開始（回り込みと同時スタート）
		TimeManager::GetInstance()->SetTimeScale(0.1f); // 1/10 速度など好みで
		slowMotionStarted_ = true;

		// ズームは少し遅らせて開始したいので、ここではタイマーだけセット
		defeatZoomTimer_ = 0.9f;    // 0.9秒後にズーム開始（好みで調整）
		defeatZoomStarted_ = false;   // 念のためリセット

		// ズーム状態もリセット
		zoomActive_ = false;
		zoomTimer_ = 0.0f;
	}


	// 次フレームの比較用
	enemyWasDead_ = enemyDeadNow;

	// ===== TimeManager から時間を取得 =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();         // スケール後
	float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime(); // スケール無し（今は未使用でもOK）

	// ===============================
	//  撃破ズームの遅延開始
	// ===============================
	if (defeatZoomTimer_ > 0.0f && !defeatZoomStarted_)
	{
		// カメラ演出と同じ「ゲーム内時間」で減らす
		defeatZoomTimer_ -= dt;

		if (defeatZoomTimer_ <= 0.0f)
		{
			using ZoomParams = CameraEffectController::ZoomParams;

			ZoomParams zoom{};
			constexpr float kZoomDuration = 0.5f; // ズームにかける時間（ゲーム内時間）

			zoom
				.UseCurrentFov(true)
				.ToFov(std::numbers::pi_v<float> / 8.0f)
				.Duration(kZoomDuration)
				.Easing(Tween::Easing::EaseOutExpo);

			cameraEffect_->StartZoom(zoom);

			defeatZoomStarted_ = true;

			// ズーム中フラグ＆残り時間セット
			zoomActive_ = true;
			zoomTimer_ = kZoomDuration;
		}
	}

	// ===============================
	//  ズーム終了を監視してスロー解除
	// ===============================
	if (zoomActive_)
	{
		// ここも dt に変更
		zoomTimer_ -= dt;

		if (zoomTimer_ <= 0.0f)
		{
			zoomActive_ = false;

			// ここでスローモーションを元に戻す
			if (slowMotionStarted_)
			{
				TimeManager::GetInstance()->SetTimeScale(1.0f); // 通常速度に戻す
				slowMotionStarted_ = false;
			}
		}
	}


	// カメラ演出の更新
	cameraEffect_->Update(followCamera.get(), dt);


	// ==========================================

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}

	collisionManager_->RegisterCollider(player_->GetMultiCollider());
	if (auto* wcol = player_->GetWeaponCollider()) {
		collisionManager_->RegisterCollider(wcol);
	}
	collisionManager_->RegisterCollider(enemy_->GetMultiCollider());

	for (auto& b : enemy_->GetDropBullets()) {
		collisionManager_->RegisterCollider(b->GetMultiCollider());
	}
	//collisionMManager_->RegisterCollider(player_->Get()->GetCollider());
	//collisionMManager_->RegisterCollider(enemy_.get());
	/*if (player_->GetBullet()) {
		auto bullet = player_->GetBullet();
		collisionMManager_->RegisterCollider(bullet);
	}*/
	/*for (const auto& areaAttack : enemy_->GetAreaAttacks()) {
		collisionMManager_->RegisterCollider(areaAttack.get());
	}
	for (const auto& bulletAttack : enemy_->GetAttackBullets()) {
		collisionMManager_->RegisterCollider(bulletAttack.get());
	}*/


	// 衝突判定と応答
	CheckAllCollisions();

	Debug();

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

	player_->BackGroundDraw();
	enemy_->BackGroundDraw();

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
	//sky->Draw();
	ground->Draw();
	player_->Draw();
	enemy_->Draw();


	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	player_->AnimationDraw();
	enemy_->AnimationDraw();


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

	ex->Draw();
	player_->ForeGroundDraw();
	enemy_->ForeGroundDraw();


	uiManager_->Draw();
	
	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	player_->ParticleDraw();
	enemy_->ParticleDraw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void GamePlayScene::Debug()
{
#ifdef _DEBUG

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など




#endif
}

void GamePlayScene::CheckAllCollisions()
{
	collisionManager_->CheckAllCollisions();
}
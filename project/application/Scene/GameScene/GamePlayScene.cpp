#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include "TimeManager.h"

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

	followCamera = std::make_unique<FollowCamera>(player_->GetCurrentCharacter(), 30.0f, 8.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize(followCamera.get());
	player_->Get()->SetCamera(followCamera.get());
	// player の現在キャラにカメラ演出コントローラを渡す
	player_->Get()->SetCameraEffectController(cameraEffect_.get());

	enemy_->Initialize();
	enemy_->SetCamera(followCamera.get());
	enemy_->SetTargetTransform(&player_->Get()->GetTransform());
	enemy_->SetCamera(followCamera.get());
	// enemy にも同じコントローラを渡す
	enemy_->SetCameraEffectController(cameraEffect_.get());

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

	AddRightDockWindow(kWindowName_MonsterControl);

	ex = std::make_unique<Sprite>();
	ex->Initialize("Resources/exp.png");
	ex->SetPosition({ 0.0f,100.0f });
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

	ex->Update();

	// カメラの更新
	camera1->Update();
	// ロックされていないときだけ followCamera を更新する
	if (!followCameraLocked_) {
		followCamera->Update();
	}

	// 敵死亡 → 撃破カメラ開始
	bool enemyDeadNow = enemy_->IsDead();  // 今の状態

	if (!enemyWasDead_ && enemyDeadNow)
	{
		// カメラ追従を止める
		followCameraLocked_ = true;

		// 回り込みの中心はプレイヤー位置（お好みで敵位置などに変えてOK）
		const Transform& playerTf = player_->Get()->GetTransform();
		Vector3 center = playerTf.translate;

		// 回り込み用パラメータ
		CameraEffectController::OrbitParams orbit{};
		float angleRad = std::numbers::pi_v<float> / 1.5f; // 回り込む角度

		orbit
			.Center(center)                       // どこを中心に回り込むか
			.Angle(angleRad)                      // 回り込む角度
			.Duration(0.6f)                       // 0.6秒かけて
			.Easing(Tween::Easing::EaseInExpo);   // お好みのEasing

		// 「回り込み＋ターゲット注視」を開始
		cameraEffect_->StartOrbitMove(
			followCamera.get(),
			orbit
		);

		// ===== スローモーション開始 =====
		// 好みで値を調整：0.1f だと1/10速度
		TimeManager::GetInstance()->SetTimeScale(0.1f);
		slowMotionStarted_ = true;

		// スロー継続時間（ゲーム内時間）をセット
		// 0.7f 秒くらいにしておくと、回り込みとほぼ同時に終わる感じ
		slowTimer_ = 0.7f;

		// ズーム制御用フラグをリセット
		defeatZoomStarted_ = false;
		zoomActive_ = false;
		zoomTimer_ = 0.0f;
	}

	// 次フレームの比較用に状態を保存
	enemyWasDead_ = enemyDeadNow;

	// ===== TimeManager から時間を取得 =====
	float dt = TimeManager::GetInstance()->GetDeltaTime();               // スケール後
	float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime(); // スケール無し（必要ならこちらで管理）

	// ===============================
	//  スローの時間を管理
	// ===============================
	if (slowMotionStarted_)
	{
		// スロー自体の長さを「ゲーム内時間」で見るなら dt
		// 現実時間ベースにしたければ unscaledDt に変更してもよい
		slowTimer_ -= dt;

		if (slowTimer_ <= 0.0f)
		{
			slowMotionStarted_ = false;

			// ===== スロー終了：タイムスケールを元に戻す =====
			TimeManager::GetInstance()->SetTimeScale(1.0f);

			// ===== ここで初めてズームを開始する =====
			if (!defeatZoomStarted_)
			{
				using ZoomParams = CameraEffectController::ZoomParams;

				ZoomParams zoom{};
				constexpr float kZoomDuration = 0.5f; // ズームにかける時間（等速）

				zoom
					.UseCurrentFov(true)                               // 現在のFOVから
					.ToFov(std::numbers::pi_v<float> / 8.0f)          // 目標FOV（小さいほど寄る）
					.Duration(kZoomDuration)
					.Easing(Tween::Easing::EaseOutExpo);

				cameraEffect_->StartZoom(zoom);

				defeatZoomStarted_ = true;
				zoomActive_ = true;
				zoomTimer_ = kZoomDuration;
			}
		}
	}

	// ===============================
	//  ズームの経過管理（タイムスケールは既に1.0）
	// ===============================
	if (zoomActive_)
	{
		zoomTimer_ -= dt;

		if (zoomTimer_ <= 0.0f)
		{
			zoomActive_ = false;
			// ここではタイムスケールはいじらない（既に通常速度）
			// 必要ならこのタイミングでフォローカメラ解除やシーン遷移なども可能
			// followCameraLocked_ = false;
			// SceneManager::GetInstance()->ChangeScene("TITLE");
		}
	}

	// カメラ演出の更新
	cameraEffect_->Update(followCamera.get(), dt);


	// ==========================================

	if (Input::GetInstance()->TriggerKey(DIK_K)) {
		PostEffectManager::GetInstance()->SetType(PostEffectType::Grayscale);
	}

	collisionManager_->RegisterCollider(player_->Get()->GetMultiCollider());
	if (auto* wcol = player_->Get()->GetWeaponCollider()) {
		collisionManager_->RegisterCollider(wcol);
	}
	collisionManager_->RegisterCollider(enemy_->GetMultiCollider());
	//collisionMAnager_->RegisterCollider(player_->Get()->GetCollider());
	//collisionMAnager_->RegisterCollider(enemy_.get());
	/*if (player_->GetBullet()) {
		auto bullet = player_->GetBullet();
		collisionMAnager_->RegisterCollider(bullet);
	}*/
	/*for (const auto& areaAttack : enemy_->GetAreaAttacks()) {
		collisionMAnager_->RegisterCollider(areaAttack.get());
	}
	for (const auto& bulletAttack : enemy_->GetAttackBulets()) {
		collisionMAnager_->RegisterCollider(bulletAttack.get());
	}*/


	// 衝突判定と応答
	CheckAllColisions();

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

	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	player_->ParticlDraw();
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

void GamePlayScene::CheckAllColisions()
{
	collisionManager_->CheckAllCollisions();
}




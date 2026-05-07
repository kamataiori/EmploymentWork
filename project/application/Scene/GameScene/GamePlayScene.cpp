#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include "engine/TimeManager.h"

#include "EnemyDropBullet.h"
#include <EnemySplitBullet.h>

#include "application/Character/CharacterBase/Enemy/AI/EnemyAIController.h"
#include "application/AI/BehaviorTree/Core/NodeResult.h"
#include <cmath>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

void GamePlayScene::Initialize()
{
	// ライト
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
	ModelManager::GetInstance()->LoadModel("Colosseum.obj");

	// カメラ
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });
	cameraEffect_ = std::make_unique<CameraEffectController>();

	// 3D オブジェクト生成
	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,0.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	Colosseum = std::make_unique<Object3d>(this);
	Colosseum->Initialize();
	Colosseum->SetModel("Colosseum.obj");
	Colosseum->SetTranslate({ 0.0f,-10.0f,0.0f });
	Colosseum->SetScale({ 0.7f,0.7f,0.7f });

	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage");

	// キャラクター生成・初期化
	player_ = std::make_unique<Player>(this);
	enemy_ = std::make_unique<Enemy>(this);

	followCamera = std::make_unique<FollowCamera>(player_.get(), 5.0f, 2.5f);
	//followCamera->SetFarClip(3000.0f);
	followCamera->SetFovY(80.0f);

	player_->Initialize();
	enemy_->Initialize();
	enemy_->SetTargetTransform(&player_->GetTransform());

	// 全オブジェクトに followCamera をセット
	sky->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	skybox->SetCamera(followCamera.get());
	stage_->SetCamera(followCamera.get());
	player_->SetCamera(followCamera.get());
	enemy_->SetCamera(followCamera.get());
	Colosseum->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	// プレイヤーの入力をロック
	player_->SetInputLocked(true);

	// イントロ演出リセット
	intro_.Reset();

	// カウントダウン用スプライト
	const float texW = 320.0f;
	const float texH = 180.0f;
	const Vector2 centerPos = { 1280.0f * 0.5f, 720.0f * 0.25f };

	for (int i = 0; i <= kCountMax; ++i) {
		countSprites_[i] = std::make_unique<Sprite>();
		countSprites_[i]->Initialize("Resources/count_" + std::to_string(i) + ".png");
		countSprites_[i]->SetSize({ texW, texH });
		countSprites_[i]->SetAnchorPoint({ 0.5f, 0.5f });
		countSprites_[i]->SetPosition(centerPos);
		countSprites_[i]->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	}

	startSprite_ = std::make_unique<Sprite>();
	startSprite_->Initialize("Resources/start.png");
	startSprite_->SetSize({ texW, texH });
	startSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	startSprite_->SetPosition(centerPos);
	startSprite_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

	// その他
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

// ================================================
// イントロ演出の更新
// ================================================
void GamePlayScene::UpdateIntro(float dt)
{
	switch (intro_.phase)
	{
	case IntroPhase::kCountdown:
	{
		intro_.countdownTimer += dt;
		if (intro_.countdownTimer >= intro_.kCountPerSec) {
			intro_.countdownTimer -= intro_.kCountPerSec;
			intro_.countdownNum--;

			if (intro_.countdownNum < 0) {
				intro_.phase = IntroPhase::kStart;
				intro_.startDisplayTimer = 0.0f;
			}
		}
		break;
	}

	case IntroPhase::kStart:
	{
		intro_.startDisplayTimer += dt;
		if (intro_.startDisplayTimer >= intro_.kStartDisplaySec) {
			intro_.phase = IntroPhase::kFinished;
			player_->SetInputLocked(false);
		}
		break;
	}

	case IntroPhase::kFinished:
	default:
		break;
	}
}

// ================================================
// イントロ UI 描画
// ================================================
void GamePlayScene::DrawIntroUI()
{
	switch (intro_.phase)
	{
	case IntroPhase::kCountdown:
	{
		int idx = intro_.countdownNum;
		if (idx >= 0 && idx <= kCountMax) {
			countSprites_[idx]->Update();
			countSprites_[idx]->Draw();
		}
		break;
	}
	case IntroPhase::kStart:
		startSprite_->Update();
		startSprite_->Draw();
		break;
	case IntroPhase::kFinished:
	default:
		break;
	}
}

// ================================================
// ライフサイクル (1) カメラ更新フェーズ
// フレームの最初に呼ばれる。停止中でも呼ばれる。
// ここでカメラの ViewMatrix/ProjectionMatrix が最新になるので、
// 以降のスカイボックスや地面の描画準備が正しい行列で行われる。
// ================================================
void GamePlayScene::UpdateCamera()
{
	// カメラの基本更新 (キャラ位置に依存しない処理)
	camera1->Update();

	// FollowCamera の更新
	// ただし followCameraLocked_ の時は LateUpdate 側で CameraEffectController が動かす
	if (!followCameraLocked_) {
		followCamera->Update();
	}

	if (stage_) {
		stage_->Update();
	}
	if (skybox) {
		skybox->Update();
	}
	if (ground) {
		ground->Update();
	}
	if (sky) {
		sky->Update();
	}
	if (Colosseum) {
		Colosseum->Update();
	}
	if (player_) {
		//player_->UpdateVisual();
	}
	if (enemy_) {
		//enemy_->UpdateVisual();
	}
}

// ================================================
// ライフサイクル (2) ゲームロジックフェーズ
// 再生中のみ呼ばれる。停止中は呼ばれない。
// ================================================
void GamePlayScene::Update()
{
	uiManager_->Update();

	if (uiManager_->IsModalActive()) {
		return;
	}

	// =========================
	// イントロ演出中
	// =========================
	if (intro_.isActive()) {
		float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

		stage_->Update();
		skybox->Update();
		ground->Update();
		sky->Update();
		Colosseum->Update();
		player_->Update();
		enemy_->UpdateVisual();

		UpdateIntro(dt);
		return;
	}

	// =========================
	// 通常ゲームの Update
	// =========================
	stage_->Update();
	skybox->Update();
	ground->Update();
	sky->Update();
	Colosseum->Update();
	player_->Update();
	enemy_->Update();
	ex->Update();

	// ===== ゲームオーバー演出 =====
	if (player_->IsDead()) {
		if (!gameOverStarted_ && player_->GetDeathTimer() <= 0.0f) {
			gameOverStarted_ = true;
			vignetteTimer_ = 0.0f;

			PostEffectManager::GetInstance()->SetType(PostEffectType::Vignette);
			PostEffectManager::GetInstance()->VignetteInitialize(
				kVignetteScale_, 0.0f, { 0.0f, 0.0f, 0.0f }
			);
		}

		if (gameOverStarted_) {
			float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
			vignetteTimer_ += dt;

			float t = vignetteTimer_ / kVignetteDuration_;
			if (t > 1.0f) t = 1.0f;

			PostEffectManager::GetInstance()->SetVignettePower(kVignettePower_ * t);

			if (t >= 1.0f) {
				PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);
				SceneManager::GetInstance()->ChangeScene("TITLE");
			}
		}
	}

	// ===== 敵死亡時のスローモーション & カメラワーク =====
	using ShakeMode = CameraEffectController::ShakeMode;
	using ZoomParams = CameraEffectController::ZoomParams;
	using MoveParams = CameraEffectController::MoveParams;

	bool enemyDeadNow = enemy_->IsDead();

	if (!enemyWasDead_ && enemyDeadNow)
	{
		followCameraLocked_ = true;

		const Transform& playerTf = player_->GetTransform();
		Vector3 center = playerTf.translate;

		CameraEffectController::OrbitParams orbit{};
		float angleRad = std::numbers::pi_v<float> / 1.5f;
		orbit
			.Center(center)
			.Angle(angleRad)
			.Duration(0.6f)
			.Easing(Tween::Easing::EaseInExpo);

		cameraEffect_->StartOrbitMove(followCamera.get(), orbit);

		TimeManager::GetInstance()->SetTimeScale(0.1f);
		slowMotionStarted_ = true;
		defeatZoomTimer_ = 0.9f;
		defeatZoomStarted_ = false;
		zoomActive_ = false;
		zoomTimer_ = 0.0f;
	}

	enemyWasDead_ = enemyDeadNow;

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

	if (defeatZoomTimer_ > 0.0f && !defeatZoomStarted_)
	{
		defeatZoomTimer_ -= dt;
		if (defeatZoomTimer_ <= 0.0f)
		{
			ZoomParams zoom{};
			constexpr float kZoomDuration = 0.5f;
			zoom
				.UseCurrentFov(true)
				.ToFov(std::numbers::pi_v<float> / 8.0f)
				.Duration(kZoomDuration)
				.Easing(Tween::Easing::EaseOutExpo);

			cameraEffect_->StartZoom(zoom);
			defeatZoomStarted_ = true;
			zoomActive_ = true;
			zoomTimer_ = kZoomDuration;
		}
	}

	if (zoomActive_)
	{
		zoomTimer_ -= dt;
		if (zoomTimer_ <= 0.0f)
		{
			zoomActive_ = false;
			if (slowMotionStarted_)
			{
				TimeManager::GetInstance()->SetTimeScale(1.0f);
				slowMotionStarted_ = false;
			}
		}
	}

	// ===== 衝突判定 =====
	collisionManager_->RegisterCollider(player_->GetMultiCollider());
	if (auto* wcol = player_->GetWeaponCollider()) {
		collisionManager_->RegisterCollider(wcol);
	}
	collisionManager_->RegisterCollider(enemy_->GetMultiCollider());

	for (auto& b : enemy_->GetDropBullets()) {
		collisionManager_->RegisterCollider(b->GetMultiCollider());
	}
	for (auto& b : enemy_->GetSplitBullets()) {
		collisionManager_->RegisterCollider(b->GetMultiCollider());
	}
	for (auto& m : enemy_->GetMinions()) {
		collisionManager_->RegisterCollider(m->GetMultiCollider());
	}

	CheckAllCollisions();
	Debug();

	if (Input::GetInstance()->TriggerKey(DIK_T)) {
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}
}

// ================================================
// ライフサイクル (3) 後処理フェーズ
// カメラエフェクト等、Update 後に処理するものをここに。
// 停止中は呼ばれない(カメラエフェクトも停止する)。
// ================================================
void GamePlayScene::LateUpdate()
{
	// カメラエフェクト (オービットムーブ、ズーム、シェイク等)
	// dt はゲーム時間(TimeScale適用後)を使う
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	cameraEffect_->Update(followCamera.get(), dt);
}

void GamePlayScene::BackGroundDraw()
{
	SpriteCommon::GetInstance()->CommonSetting();
	player_->BackGroundDraw();
	enemy_->BackGroundDraw();
}

void GamePlayScene::Draw()
{
	Object3dCommon::GetInstance()->CommonSetting();
	ground->Draw();
	Colosseum->Draw();
	player_->Draw();
	enemy_->Draw();

	Skinning::GetInstance()->CommonSetting();
	player_->AnimationDraw();
	enemy_->AnimationDraw();
}

void GamePlayScene::ForeGroundDraw()
{
	SpriteCommon::GetInstance()->CommonSetting();

	if (intro_.isActive()) {
		DrawIntroUI();
		return;
	}

	//ex->Draw();
	player_->ForeGroundDraw();
	enemy_->ForeGroundDraw();
	uiManager_->Draw();
	player_->ParticleDraw();
	enemy_->ParticleDraw();
}

void GamePlayScene::Debug()
{
#ifdef _DEBUG
	if (!IsDockedImGuiEnabled()) return;

	/*ImGui::SetNextWindowPos(ImVec2(10, 80), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(320, 220), ImGuiCond_Once);
	ImGui::Begin("BT デバッグ", nullptr, ImGuiWindowFlags_None);

	if (enemy_) {
		auto* ai = enemy_->GetAIController();
		if (ai) {
			auto info = ai->GetDebugInfo();

			const char* resultStr = "Idle";
			ImVec4 resultColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			switch (info.rootResult) {
			case NodeResult::Running:
				resultStr = "Running"; resultColor = { 0.4f,1.0f,0.4f,1.0f }; break;
			case NodeResult::Success:
				resultStr = "Success"; resultColor = { 0.4f,0.8f,1.0f,1.0f }; break;
			case NodeResult::Fail:
				resultStr = "Fail";    resultColor = { 1.0f,0.3f,0.3f,1.0f }; break;
			default: break;
			}
			ImGui::Text("BT 状態:");
			ImGui::SameLine();
			ImGui::TextColored(resultColor, "%s", resultStr);
			ImGui::Separator();

			ImGui::Text("実行中ステート:");
			ImGui::SameLine();
			if (info.runningStateName.empty() || info.runningStateName == "(なし)") {
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(なし)");
			}
			else {
				ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f),
					"%s", info.runningStateName.c_str());
			}
			ImGui::Separator();

			ImGui::Text("BlackBoard:");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
				"%s", info.blackboardInfo.c_str());
			ImGui::Separator();

			if (enemy_->GetTargetTransform()) {
				Vector3 diff = enemy_->GetTargetTransform()->translate
					- enemy_->GetTransform().translate;
				diff.y = 0.0f;
				float dist = std::sqrt(diff.x * diff.x + diff.z * diff.z);
				ImGui::Text("ターゲットまでの距離: %.2f m", dist);
			}
		}
		else {
			ImGui::TextDisabled("AIController なし");
		}
	}
	else {
		ImGui::TextDisabled("Enemy なし");
	}

	ImGui::End();*/
#endif
}

void GamePlayScene::CheckAllCollisions()
{
	collisionManager_->CheckAllCollisions();
}

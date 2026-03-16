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

	// カメラ
	camera1->SetTranslate({ 0.0f, 0.0f, -20.0f });
	cameraEffect_ = std::make_unique<CameraEffectController>();

	// 3D オブジェクト生成（SetCamera より前に必ず生成）
	skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,0.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage");

	// キャラクター生成・初期化
	player_ = std::make_unique<Player>(this);
	enemy_ = std::make_unique<Enemy>(this);

	followCamera = std::make_unique<FollowCamera>(player_.get(), 9.5f, 2.2f);
	followCamera->SetFarClip(2000.0f);
	followCamera->SetFovY(75.0f);

	player_->Initialize();
	enemy_->Initialize();
	enemy_->SetTargetTransform(&player_->GetTransform());

	// 全オブジェクトに followCamera をセット（イントロから最初から通常カメラ）
	sky->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	skybox->SetCamera(followCamera.get());
	stage_->SetCamera(followCamera.get());
	player_->SetCamera(followCamera.get());
	enemy_->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	// プレイヤーの入力をロック（イントロ終わるまで動かせない）
	player_->SetInputLocked(true);

	// イントロ演出リセット
	intro_.Reset();

	// ================================================
	// カウントダウン用スプライト
	// 画面中央より少し上に配置
	// "Resources/count_0.png" 〜 "Resources/count_5.png" を想定
	// ※ 実際のリソースパスに合わせてください
	// ================================================
	// 画面サイズ 1280x720 の中央より少し上
	// テクスチャサイズ 320x180 の中心を画面中央より少し上に合わせる
	// アンカーポイントを (0.5, 0.5) にして SetPosition で中心座標を指定する
	const float texW = 320.0f;
	const float texH = 180.0f;
	const Vector2 centerPos = { 1280.0f * 0.5f, 720.0f * 0.25f };

	for (int i = 0; i <= kCountMax; ++i) {
		countSprites_[i] = std::make_unique<Sprite>();
		countSprites_[i]->Initialize("Resources/count_" + std::to_string(i) + ".png");
		countSprites_[i]->SetSize({ texW, texH });
		countSprites_[i]->SetAnchorPoint({ 0.5f, 0.5f }); // 中心基準
		countSprites_[i]->SetPosition(centerPos);
		countSprites_[i]->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤
	}

	startSprite_ = std::make_unique<Sprite>();
	startSprite_->Initialize("Resources/start.png");
	startSprite_->SetSize({ texW, texH });
	startSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 中心基準
	startSprite_->SetPosition(centerPos);
	startSprite_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤

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
		// --------------------------------------------------
		// 5→0 カウントダウン
		// --------------------------------------------------
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

	// --------------------------------------------------
	// "START!" 表示
	// --------------------------------------------------
	case IntroPhase::kStart:
	{
		intro_.startDisplayTimer += dt;
		if (intro_.startDisplayTimer >= intro_.kStartDisplaySec) {
			intro_.phase = IntroPhase::kFinished;
			player_->SetInputLocked(false); // 入力ロック解除
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
		player_->Update();
		enemy_->UpdateVisual(); // AI非動作・見た目のみ（攻撃させない）
		followCamera->Update();

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
	player_->Update();
	enemy_->Update();
	ex->Update();

	camera1->Update();
	if (!followCameraLocked_) {
		followCamera->Update();
	}

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

	cameraEffect_->Update(followCamera.get(), dt);

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

void GamePlayScene::BackGroundDraw()
{
	SpriteCommon::GetInstance()->CommonSetting();
	player_->BackGroundDraw();
	enemy_->BackGroundDraw();
}

void GamePlayScene::Draw()
{
	Object3dCommon::GetInstance()->CommonSetting();
	sky->Draw();
	ground->Draw();
	player_->Draw();
	enemy_->Draw();

	Skinning::GetInstance()->CommonSetting();
	player_->AnimationDraw();
	enemy_->AnimationDraw();
}

void GamePlayScene::ForeGroundDraw()
{
	SpriteCommon::GetInstance()->CommonSetting();

	// イントロ演出中は専用 UI のみ描画
	if (intro_.isActive()) {
		DrawIntroUI();
		return;
	}

	// 通常ゲームの前景描画
	ex->Draw();
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

	// ===== BT デバッグウィンドウ ===== ← ここから追加
	ImGui::SetNextWindowPos(ImVec2(10, 80), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(320, 220), ImGuiCond_Once);
	ImGui::Begin("BT デバッグ", nullptr, ImGuiWindowFlags_None);

	if (enemy_) {
		auto* ai = enemy_->GetAIController();
		if (ai) {
			auto info = ai->GetDebugInfo();

			// BT状態
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

			// 実行中ステート
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

			// BlackBoard情報
			ImGui::Text("BlackBoard:");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
				"%s", info.blackboardInfo.c_str());
			ImGui::Separator();

			// 距離情報
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

	ImGui::End();
	// ===== ここまで追加 =====
#endif
}

void GamePlayScene::CheckAllCollisions()
{
	collisionManager_->CheckAllCollisions();
}
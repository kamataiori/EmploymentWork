#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include "engine/TimeManager.h"

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

	followCamera = std::make_unique<FollowCamera>(player_->GetCurrentCharacter(), 30.0f, 8.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize(followCamera.get());
	player_->Get()->SetCamera(followCamera.get());
	/*followCamera->SetTarget(player_->Get());*/

	enemy_->Initialize();
	enemy_->SetCamera(followCamera.get());
	enemy_->SetTargetTransform(&player_->Get()->GetTransform());
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

	// PauseScreen を UI として登録
	auto pauseScreen = std::make_unique<PauseScreen>();
	pauseScreen->Initialize({ 1280.0f, 720.0f }, "TITLE");
	pauseScreen->SetUIManager(uiManager_.get());

	uiManager_->Add(std::move(pauseScreen));

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
	if (uiManager_->IsPaused()) {
		return;
	}
	//	// =========================
	//// Pause トグル（ESC）
	//// =========================
	//	if (Input::GetInstance()->PushKey(DIK_ESCAPE)) {
	//		if (!escLock_) {
	//			escLock_ = true;
	//
	//			// ここが重要：状態に応じて分岐
	//			if (!isPaused_) {
	//				// ゲーム中 -> ポーズ開始
	//				EnterPause();
	//				pauseView_ = PauseView::Menu;
	//			}
	//			else {
	//				// ポーズ中
	//				if (!isPaused_) {
	//					EnterPause();
	//					pauseView_ = PauseView::Menu;
	//				}
	//				else {
	//					HandlePauseBack(); // ここに統一
	//				}
	//
	//			}
	//		}
	//	}
	//	else {
	//		escLock_ = false;
	//	}
	//
	//	// ポーズ中はゲーム更新停止、UIだけ更新
	//	if (isPaused_) {
	//
	//		// アニメは時間停止の影響を受けないように unscaled を使う
	//		const float udt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
	//
	//		// Enter/Exit中はアニメだけ進める（クリック受付はしない方が安全）
	//		if (pauseAnimState_ == PauseAnimState::Entering || pauseAnimState_ == PauseAnimState::Exiting) {
	//			UpdatePauseEnterExitAnim(udt);
	//			return;
	//		}
	//
	//		// Idleなら通常のポーズUI操作
	//		UpdatePauseMouseUI();
	//		return;
	//	}
	//
	//


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
		const Transform& playerTf = player_->Get()->GetTransform();
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
	//// =========================
	//// Pause 画面（ForeGroundDrawで描画してOK）
	//// =========================
	//// まずゲーム中のESCヒント（操作不可）
	//if (!isPaused_) {
	//	if (escHint_) {
	//		escHint_->Draw();
	//	}
	//}

	//// ポーズ中UI
	//if (isPaused_) {
	//	pauseBlack_->Draw();
	//	pauseMenu_->Draw();

	//	if (pauseView_ == PauseView::Menu) {
	//		pauseOpe_->Draw();
	//		pauseBackTitle_->Draw();
	//	}
	//	else {
	//		pauseExp_->Draw();
	//	}

	//	// ポーズ中は操作可能なesc（ホバー/クリック）を出す
	//	pauseEsc_->Draw();
	//}

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

void GamePlayScene::CheckAllCollisions()
{
	collisionManager_->CheckAllCollisions();
}

//void GamePlayScene::EnterPause()
//{
//	if (isPaused_) return;
//	isPaused_ = true;
//
//	// ゲーム時間を止める
//	TimeManager::GetInstance()->SetTimeScale(0.0f);
//
//	pauseView_ = PauseView::Menu;
//	BeginPauseEnterAnim();
//
//
//	// 見た目更新（1回でOK）
//	UpdatePauseSprites();
//}
//
//void GamePlayScene::ExitPause()
//{
//	if (!isPaused_) return;
//	isPaused_ = false;
//
//	// ゲーム時間を戻す
//	TimeManager::GetInstance()->SetTimeScale(1.0f);
//}
//
//void GamePlayScene::UpdatePauseSprites()
//{
//	// 真ん中ちょい上（好みで調整）
//	const float centerX = 1280.0f * 0.5f;
//	const float menuY = 220.0f;
//
//	// menu: 256x128
//	pauseMenu_->SetPosition({ centerX, menuY });
//
//	// ope: menuの下
//	// menu半分(64) + 間隔(24) + ope半分(32) = 120
//	const float opeY = menuY + 64.0f + 24.0f + 32.0f;
//	pauseOpe_->SetPosition({ centerX, opeY });
//
//	// backTitle: opeの下
//	// ope半分(32) + 間隔(16) + back半分(32) = 80
//	const float backY = opeY + 32.0f + 16.0f + 32.0f;
//	pauseBackTitle_->SetPosition({ centerX, backY });
//
//	// 目標位置（中央）を保存
//	menuTargetPos_ = pauseMenu_->GetPosition();
//	opeTargetPos_ = pauseOpe_->GetPosition();
//	backTargetPos_ = pauseBackTitle_->GetPosition();
//
//	// 開始位置（左から中央へ）
//	// 画面外から来る感じにしたいので、Xをマイナス側へ
//	menuStartPos_ = { -256.0f, menuTargetPos_.y };
//	opeStartPos_ = { -256.0f, opeTargetPos_.y };
//	backStartPos_ = { -256.0f, backTargetPos_.y };
//
//
//	pauseMenu_->Update();
//	pauseOpe_->Update();
//	pauseBackTitle_->Update();
//}
//
//bool GamePlayScene::HitTestSprite(const Sprite* sp, const POINT& mouse) const
//{
//	if (!sp) return false;
//
//	const Vector2 pos = sp->GetPosition();
//	const Vector2 size = sp->GetSize();
//	const Vector2 anchor = sp->GetAnchorPoint();
//
//	const float left = pos.x - size.x * anchor.x;
//	const float top = pos.y - size.y * anchor.y;
//	const float right = left + size.x;
//	const float bottom = top + size.y;
//
//	const float mx = static_cast<float>(mouse.x);
//	const float my = static_cast<float>(mouse.y);
//
//	return (mx >= left && mx <= right && my >= top && my <= bottom);
//}
//
//void GamePlayScene::UpdatePauseMouseUI()
//{
//	const POINT m = Input::GetInstance()->GetMousePosition();
//
//	// -------------------------
//	// ESC（右下）ホバー＆クリック：どの画面でも有効
//	// -------------------------
//	const bool hoverEsc = HitTestSprite(pauseEsc_.get(), m);
//	pauseEsc_->SetSize(hoverEsc ? escHoverSize_ : escBaseSize_);
//	pauseEsc_->Update();
//
//	if (hoverEsc && Input::GetInstance()->TriggerMouseButton(0)) {
//		HandlePauseBack(); // ★クリックで戻る/再開
//		return;
//	}
//
//	// -------------------------
//	// ここから下は「ポーズメニュー中」だけの処理
//	// -------------------------
//	if (pauseView_ == PauseView::Menu) {
//
//		const bool hoverOpe = HitTestSprite(pauseOpe_.get(), m);
//		const bool hoverBack = HitTestSprite(pauseBackTitle_.get(), m);
//
//		pauseOpe_->SetSize(hoverOpe ? hoverSize_ : opeBaseSize_);
//		pauseBackTitle_->SetSize(hoverBack ? hoverSize_ : backBaseSize_);
//		pauseOpe_->Update();
//		pauseBackTitle_->Update();
//
//		if (Input::GetInstance()->TriggerMouseButton(0)) {
//
//			if (hoverBack) {
//				ExitPause();
//				pauseView_ = PauseView::Menu;
//
//				TransitionRequest req{};
//				req.type = TransitionType::Fade;
//				req.fadeOutSec = 0.3f;
//				req.fadeInSec = 0.3f;
//
//				SceneManager::GetInstance()->RequestChangeScene("TITLE", req);
//				return;
//			}
//
//			if (hoverOpe) {
//				pauseView_ = PauseView::Explain;
//				return;
//			}
//		}
//	}
//	else {
//		// Explain中：ESC/escクリックで戻るので、ここは何もしなくてOK
//	}
//}
//
//
//void GamePlayScene::HandlePauseBack()
//{
//	if (!isPaused_) return;
//
//	if (pauseView_ == PauseView::Explain) {
//		// 操作説明中 -> ポーズメニューへ
//		pauseView_ = PauseView::Menu;
//		return;
//	}
//
//	// ポーズメニュー中 -> 退出アニメ開始（即ExitPauseしない）
//	BeginPauseExitAnim();
//}
//
//
//void GamePlayScene::BeginPauseEnterAnim()
//{
//	pauseAnimState_ = PauseAnimState::Entering;
//	pauseAnimTime_ = 0.0f;
//
//	// まず左側（開始位置）に置く
//	pauseMenu_->SetPosition(menuStartPos_);
//	pauseOpe_->SetPosition(opeStartPos_);
//	pauseBackTitle_->SetPosition(backStartPos_);
//
//	pauseMenu_->Update();
//	pauseOpe_->Update();
//	pauseBackTitle_->Update();
//}
//
//void GamePlayScene::BeginPauseExitAnim()
//{
//	// すでに退出中なら二重開始しない
//	if (pauseAnimState_ == PauseAnimState::Exiting) return;
//
//	pauseAnimState_ = PauseAnimState::Exiting;
//	pauseAnimTime_ = 0.0f;
//}
//
//// entering=true: 左→中央
//// entering=false: 中央→左
//void GamePlayScene::ApplySlideAnimToSprites(float t, bool entering)
//{
//	// TweenEasing.h を使う（あなたが持ってるやつ）
//	t = Tween::Easing::EaseInOutCubic(std::clamp(t, 0.0f, 1.0f));
//
//	auto lerpVec2 = [](const Vector2& a, const Vector2& b, float tt) -> Vector2 {
//		return { a.x + (b.x - a.x) * tt, a.y + (b.y - a.y) * tt };
//		};
//
//	// entering時：start→target
//	// exiting時：target→start
//	const Vector2 menuA = entering ? menuStartPos_ : menuTargetPos_;
//	const Vector2 menuB = entering ? menuTargetPos_ : menuStartPos_;
//
//	const Vector2 opeA = entering ? opeStartPos_ : opeTargetPos_;
//	const Vector2 opeB = entering ? opeTargetPos_ : opeStartPos_;
//
//	const Vector2 backA = entering ? backStartPos_ : backTargetPos_;
//	const Vector2 backB = entering ? backTargetPos_ : backStartPos_;
//
//	pauseMenu_->SetPosition(lerpVec2(menuA, menuB, t));
//	pauseOpe_->SetPosition(lerpVec2(opeA, opeB, t));
//	pauseBackTitle_->SetPosition(lerpVec2(backA, backB, t));
//
//	pauseMenu_->Update();
//	pauseOpe_->Update();
//	pauseBackTitle_->Update();
//}
//
//void GamePlayScene::UpdatePauseEnterExitAnim(float unscaledDt)
//{
//	// 3つが順番に出るように、stagger（遅延）を入れる
//	pauseAnimTime_ += unscaledDt;
//
//	const bool entering = (pauseAnimState_ == PauseAnimState::Entering);
//	const float dur = entering ? pauseEnterSec_ : pauseExitSec_;
//
//	// それぞれの開始時刻（上から順）
//	const float tMenuStart = 0.0f;
//	const float tOpeStart = pauseStaggerSec_;
//	const float tBackStart = pauseStaggerSec_ * 2.0f;
//
//	auto localT = [&](float start) -> float {
//		const float x = (pauseAnimTime_ - start) / std::max(dur, 0.0001f);
//		return std::clamp(x, 0.0f, 1.0f);
//		};
//
//	// 個別に補間するため、Applyを3回に分ける（簡単＆確実）
//	// menu
//	{
//		float t = Tween::Easing::EaseInOutCubic(localT(tMenuStart));
//		auto lerp = [](const Vector2& a, const Vector2& b, float tt) -> Vector2 {
//			return { a.x + (b.x - a.x) * tt, a.y + (b.y - a.y) * tt };
//			};
//		const Vector2 a = entering ? menuStartPos_ : menuTargetPos_;
//		const Vector2 b = entering ? menuTargetPos_ : menuStartPos_;
//		pauseMenu_->SetPosition(lerp(a, b, t));
//	}
//	// ope
//	{
//		float t = Tween::Easing::EaseInOutCubic(localT(tOpeStart));
//		auto lerp = [](const Vector2& a, const Vector2& b, float tt) -> Vector2 {
//			return { a.x + (b.x - a.x) * tt, a.y + (b.y - a.y) * tt };
//			};
//		const Vector2 a = entering ? opeStartPos_ : opeTargetPos_;
//		const Vector2 b = entering ? opeTargetPos_ : opeStartPos_;
//		pauseOpe_->SetPosition(lerp(a, b, t));
//	}
//	// backTitle
//	{
//		float t = Tween::Easing::EaseInOutCubic(localT(tBackStart));
//		auto lerp = [](const Vector2& a, const Vector2& b, float tt) -> Vector2 {
//			return { a.x + (b.x - a.x) * tt, a.y + (b.y - a.y) * tt };
//			};
//		const Vector2 a = entering ? backStartPos_ : backTargetPos_;
//		const Vector2 b = entering ? backTargetPos_ : backStartPos_;
//		pauseBackTitle_->SetPosition(lerp(a, b, t));
//	}
//
//	pauseMenu_->Update();
//	pauseOpe_->Update();
//	pauseBackTitle_->Update();
//
//	// アニメ完了判定（最後の要素が終わる時間）
//	const float endTime = tBackStart + dur;
//
//	if (pauseAnimTime_ >= endTime) {
//		if (pauseAnimState_ == PauseAnimState::Entering) {
//			pauseAnimState_ = PauseAnimState::Idle;
//		}
//		else if (pauseAnimState_ == PauseAnimState::Exiting) {
//			// 退出アニメが終わったらゲーム再開
//			pauseAnimState_ = PauseAnimState::Idle;
//			ExitPause();
//		}
//	}
//}

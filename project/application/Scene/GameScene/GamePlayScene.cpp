#include "GamePlayScene.h"
#include <Input.h>
#include "SceneManager.h"
#include "engine/UI/UITexture.h"
#include "engine/UI/PlayerHpPipBar.h"
#include "engine/UI/NumberDisplay.h"
#include "CursorService.h"
#include <OffscreenRendering.h>
#include <MyGame.h>
#include "engine/TimeManager.h"

#include "Flow/States/IntroState.h"

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

	// レイヤー別ポストエフェクト：
	//   オブジェクト層(World) = そのまま / パーティクル層(Particle) = Bloom
	//   （かける対象を変えたいときはこの数行の LayerType を書き換えるだけ）
	auto* pe = PostEffectManager::GetInstance();
	pe->SetLayerType(RenderLayerId::World, PostEffectType::Blur5x5);
	pe->SetLayerType(RenderLayerId::Particle, PostEffectType::Bloom);
	if (auto* particleFx = pe->GetEffect(RenderLayerId::Particle)) {
		particleFx->SetBloomThreshold(bloomThreshold_);   // 明るいと判定する輝度
		particleFx->SetBloomIntensity(bloomIntensity_);   // Bloomの強さ
		particleFx->SetBloomIterations(bloomIterations_); // ぼかしの反復回数
	}

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

	// ステージ（stage.json が持つ stage.obj）は SceneController に一本化する。
	// 描画も押し戻し用BVHも、この単一インスタンスから作られる（見た目と当たり判定が完全一致）。
	stage_ = std::make_unique<SceneController>(this);
	stage_->LoadScene("stage");
	// ステージのメッシュから押し戻し用のBVHコライダーを構築（静的なので1回だけ）
	stage_->BuildStageCollider();

	// キャラクター生成・初期化
	player_ = std::make_unique<Player>(this);
	enemy_ = std::make_unique<Enemy>(this);

	// 第1波の群れ（波状に登場）。追尾対象としてプレイヤーの Transform を渡す。
	swarmController_ = std::make_unique<SwarmController>();
	swarmController_->Initialize(this, &player_->GetTransform());

	// 前哨の敵4体（ステージ中央を囲む正方形の四隅）。
	sentinelField_ = std::make_unique<SentinelField>();
	sentinelField_->Initialize(this, { 0.0f, 0.0f, 0.0f }, kSentinelFieldHalfExtent_);
	// 登場前はイントロ中に見えないよう地中へ沈めておく（SentinelBattleで迫り上がる）
	sentinelField_->SetHiddenUnderground();

	// 中央のボスのコア（原点）。Sentinel 全滅まで破壊不可にしておく。
	centerCore_ = std::make_unique<Sentinel>(this);
	centerCore_->InitializeSentinel({ 0.0f, 0.0f, 0.0f }, kCenterCoreScale_);
	centerCore_->SetHittable(false);

	followCamera = std::make_unique<FollowCamera>(player_.get(), 5.0f, 2.5f);
	followCamera->SetFovY(80.0f);

	player_->Initialize();
	enemy_->Initialize();
	enemy_->SetTargetTransform(&player_->GetTransform());
	// スキル2（突進乱舞）の攻撃対象供給元としてボス（自身＋配下の雑魚を束ねる）を注入
	player_->SetEnemyTargetProvider(enemy_.get());
	player_->SetCameraEffect(cameraEffect_.get());
	enemy_->SetCameraEffect(cameraEffect_.get());

	// 与ダメージ数値ポップアップ（敵の右上に表示）を生成し、敵へ注入する
	damagePopup_ = std::make_unique<DamagePopupManager>();
	damagePopup_->SetCamera(followCamera.get());
	enemy_->SetDamagePopupSink(damagePopup_.get());
	swarmController_->SetDamagePopupSink(damagePopup_.get());
	sentinelField_->SetDamagePopupSink(damagePopup_.get());
	centerCore_->SetDamagePopupSink(damagePopup_.get());

	// 全オブジェクトに followCamera をセット
	sky->SetCamera(followCamera.get());
	ground->SetCamera(followCamera.get());
	skybox->SetCamera(followCamera.get());
	stage_->SetCamera(followCamera.get());
	player_->SetCamera(followCamera.get());
	enemy_->SetCamera(followCamera.get());
	swarmController_->SetCamera(followCamera.get());
	sentinelField_->SetCamera(followCamera.get());
	centerCore_->SetCamera(followCamera.get());
	DrawLine::GetInstance()->SetCamera(followCamera.get());

	// その他
	collisionManager_ = std::make_unique<CollisionManager>();

	uiManager_ = std::make_unique<UIManager>();

	// 右下：プレイヤーの攻撃／スキルUI（ゲージ台座）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/gauge_bar_ui.png";
		d.size    = { 256.0f, 256.0f };
		d.anchor  = { 1.0f, 1.0f };                       // 右下基準
		d.pos     = { 1280.0f - 48.0f, 720.0f - 0.0f };   // 右下から少し内側＆下げる
		d.layer   = 100;                                  // ポーズUI(100000)より下
		uiManager_->Add(UITexture::Create(d));
	}

	// ブーメランアイコンの左隣：フレイムダンス（スキル）アイコン
	{
		UITexture::Desc d{};
		d.texPath = "Resources/flameDance_ui.png";
		d.size    = { 64.0f, 64.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 856.0f, 592.0f };   // ブーメランとの隙間を、ベース(左端976)とブーメラン(右端964)の隙間12pxに合わせる
		d.layer   = 101;                  // ゲージ台座(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// フレイムダンスアイコンの下：操作キー表示（V）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/V_ui.png";
		d.size    = { 32.0f, 32.0f };     // E_ui と同じサイズ
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 856.0f, 660.0f };   // x はフレイムダンスの真下、y は E_ui と同じ高さ
		d.layer   = 102;                  // フレイムダンスアイコン(101)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// 剣アイコンの左側：ブーメラン（スキル）アイコン
	{
		UITexture::Desc d{};
		d.texPath = "Resources/boomerang_ui.png";
		d.size    = { 64.0f, 64.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 932.0f, 592.0f };   // 剣(左端996)の左側、高さは剣と同じ
		d.layer   = 101;                  // ゲージ台座(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// ブーメランアイコンの下：操作キー表示（E）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/E_ui.png";
		d.size    = { 32.0f, 32.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 932.0f, 660.0f };   // x はブーメランと同じ、y は左キーと同じ高さ
		d.layer   = 102;                  // ブーメランアイコン(101)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// ゲージ台座の上に重ねる：通常攻撃アイコン（剣）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/sword_ui.png";
		d.size    = { 128.0f, 128.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 1060.0f, 592.0f };  // ゲージ中央(約1104,592)の高さ・少し左寄り
		d.layer   = 101;                  // ゲージ台座(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// 剣アイコンの右側：無限マーク
	{
		UITexture::Desc d{};
		d.texPath = "Resources/infinite_ui.png";
		d.size    = { 64.0f, 64.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 1170.0f, 592.0f };  // 剣(右端1124)の右側、高さは剣と同じ
		d.layer   = 101;                  // ゲージ台座(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// 上記2つの下に重ねる：操作キー表示（左クリック）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/leftKey_ui.png";
		d.size    = { 64.0f, 64.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 1104.0f, 660.0f };  // x はゲージ中央、y は剣アイコンの下
		d.layer   = 102;                  // 剣アイコン(101)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// 画面中央：アルティメットのリング
	{
		UITexture::Desc d{};
		d.texPath = "Resources/ultRing_ui.png";
		d.size    = { 126.0f, 126.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 640.0f, 592.0f };   // x は画面ど真ん中、y は剣アイコンと同じ高さ
		d.layer   = 101;                  // ゲージ台座(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// アルティメットリングの下：操作キー表示（Q）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/Q_ui.png";
		d.size    = { 32.0f, 32.0f };
		d.anchor  = { 0.5f, 0.5f };       // 中心基準
		d.pos     = { 640.0f, 680.0f };   // x はリングと同じ、y は他のキーと同じ高さ
		d.layer   = 102;                  // リング(101)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// 左下：プレイヤーのHP（HP.png 1枚＝15HP、最大275 → 19枚を横並び）
	{
		PlayerHpPipBar::CreateDesc d{};
		d.pipTexPath = "Resources/HP.png";
		d.startPos   = { 64.0f, 640.0f }; // 左端。health_ui と重ならないよう少し下げる
		d.pipSize    = { 8.0f, 24.0f };
		d.spacing    = 2.0f;
		d.maxHp      = 275;
		d.hpPerPip   = 15;
		d.layer      = 100;
		uiManager_->Add(PlayerHpPipBar::Create(d));
	}

	// HPバーの一番左に重ねる：HPアイコン／ラベル
	{
		UITexture::Desc d{};
		d.texPath = "Resources/health_ui.png";
		d.size    = { 64.0f, 64.0f };
		d.anchor  = { 0.0f, 0.5f };       // 左端・縦中央基準
		d.pos     = { 48.0f, 592.0f };    // HP の一番左、高さはゲージ台座と同じ y=592
		d.layer   = 101;                  // HPピップ(100)の上
		uiManager_->Add(UITexture::Create(d));
	}

	// health_ui の右隣：現在HP（数字・右寄せで右端固定）
	{
		NumberDisplay::CreateDesc d{};
		d.dir       = "Resources/number/";
		d.pos       = { 174.0f, 592.0f }; // 右端を x=174 に固定（桁数が変わっても右端不動）
		d.digitSize = { 18.0f, 24.0f };
		d.spacing   = 2.0f;
		d.align     = NumberDisplay::Align::Right;
		d.source    = NumberDisplay::Source::CurrentHp;
		d.layer     = 101;
		uiManager_->Add(NumberDisplay::Create(d));
	}

	// 現在HPとmaxHPの間：仕切り（縦棒）
	{
		UITexture::Desc d{};
		d.texPath = "Resources/verticalBar_ui.png";
		d.size    = { 6.0f, 28.0f };
		d.anchor  = { 0.0f, 0.5f };       // 左端・縦中央
		d.pos     = { 182.0f, 592.0f };   // 現在HP(右端174)とmaxHPの間
		d.layer   = 101;
		uiManager_->Add(UITexture::Create(d));
	}

	// verticalBar の右隣：最大HP（数字・左寄せ）
	{
		NumberDisplay::CreateDesc d{};
		d.dir       = "Resources/number/";
		d.pos       = { 196.0f, 592.0f }; // 仕切りの右から左寄せで並べる
		d.digitSize = { 18.0f, 24.0f };
		d.spacing   = 2.0f;
		d.align     = NumberDisplay::Align::Left;
		d.source    = NumberDisplay::Source::MaxHp;
		d.layer     = 101;
		uiManager_->Add(NumberDisplay::Create(d));
	}

	auto pause = std::make_unique<PauseScreen>();
	pause->Initialize({ 1280.0f, 720.0f }, "TITLE");
	pauseScreenRef_ = pause.get();  // 所有は uiManager_、こちらは状態監視のための非所有参照
	uiManager_->Add(std::move(pause));

	// 初期状態はポーズではないので、シーン要求どおりカーソル非表示+閉じ込め
	wasPausedLastFrame_ = false;

	// バトルの進行を開始（最初の局面 = 開始演出）
	BuildFlowContext();
	flow_.ChangeState(std::make_unique<IntroState>());
}

void GamePlayScene::Finalize()
{
}

// ================================================
// State が触る「場」を組み立てる。
// 所有はすべてこのシーン側にあり、context_ に入るのは非所有ポインタだけ。
// ================================================
void GamePlayScene::BuildFlowContext()
{
	context_.player = player_.get();
	context_.boss = enemy_.get();
	context_.swarm = swarmController_.get();
	context_.sentinels = sentinelField_.get();
	context_.centerCore = centerCore_.get();

	context_.stage = stage_.get();
	context_.skybox = skybox.get();
	context_.ground = ground.get();
	context_.sky = sky.get();

	context_.followCamera = followCamera.get();
	context_.cameraEffect = cameraEffect_.get();

	context_.collisionManager = collisionManager_.get();
	context_.uiManager = uiManager_.get();
	context_.damagePopup = damagePopup_.get();

	context_.flow = &flow_;
}

void GamePlayScene::SyncCursorWithPauseState()
{
	if (!pauseScreenRef_) return;

	const bool isPausedNow = pauseScreenRef_->IsPaused();
	if (isPausedNow == wasPausedLastFrame_) {
		return; // 状態が変わっていなければ何もしない
	}
	wasPausedLastFrame_ = isPausedNow;

	SceneManager* sm = GetSceneManager();
	if (!sm) return;
	CursorService* cursor = sm->GetCursorService();
	if (!cursor) return;

	if (isPausedNow) {
		// ポーズ中: メニュー操作のためカーソル表示・閉じ込め解除
		cursor->ApplySceneRequest(true, false);
	} else {
		// ポーズ解除: シーン本来の設定に戻す (非表示+閉じ込め)
		cursor->ApplySceneRequest(ShouldShowCursor(), ShouldConfineCursor());
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
	// 決着演出中(followCameraLocked)は LateUpdate 側で CameraEffectController が動かす
	if (!context_.followCameraLocked) {
		followCamera->Update();
	}
}

// ================================================
// ライフサイクル (2) ゲームロジックフェーズ
// 局面によらず必要なもの（UI・ポーズ・ダメージ表示）だけをここで捌き、
// バトルの中身は現在の局面へ委譲する。
// ================================================
void GamePlayScene::Update()
{
	uiManager_->Update();

	// プレイヤーHPを左下のHP UI へ反映（PlayerHpPipBar が ApplyData で受け取る）
	UIElement::UIData hpData{};
	hpData.hp = static_cast<float>(player_->GetHp());
	hpData.maxHp = static_cast<float>(player_->GetMaxHp());
	uiManager_->ApplyDataToAll(hpData);

	// ポーズ状態の変化を検知してカーソル設定を切り替える
	SyncCursorWithPauseState();

	// ポーズ中はバトルを止める
	if (uiManager_->IsModalActive()) {
		return;
	}

	// 与ダメージ数値ポップアップの更新（浮き上がり・フェード・寿命）
	if (damagePopup_) {
		damagePopup_->Update(TimeManager::GetInstance()->GetDeltaTime());
	}

	// 今の局面（開始演出 / 戦闘 / 決着）を1回進める
	flow_.Update(context_);

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
	// 3Dの見た目は局面によらず同じ（演出中も戦闘中もステージとキャラは映る）
	Object3dCommon::GetInstance()->CommonSetting();
	ground->Draw();
	// ステージは SceneController が描画する。BVHもこの同一メッシュから作られる。
	stage_->Draw();
	player_->Draw();
	enemy_->Draw();
	swarmController_->Draw();
	sentinelField_->Draw();
	centerCore_->Draw();

	Skinning::GetInstance()->CommonSetting();
	player_->AnimationDraw();
	enemy_->AnimationDraw();
}

void GamePlayScene::ForeGroundDraw()
{
	// Canvas方式：ここは World（前景スプライト）のみ。
	// パーティクルは ParticleDraw()、UIは UIDraw() へ分離した。
	SpriteCommon::GetInstance()->CommonSetting();
	flow_.ForeGroundDraw(context_);
}

void GamePlayScene::ParticleDraw()
{
	// パーティクル専用Canvasへ描画される
	flow_.ParticleDraw(context_);
}

void GamePlayScene::UIDraw()
{
	// 合成後のバックバッファへ直接描画（ポストエフェクト対象外）
	SpriteCommon::GetInstance()->CommonSetting();
	flow_.UIDraw(context_);
}

void GamePlayScene::Debug()
{
#ifdef _DEBUG
	if (!IsDockedImGuiEnabled()) return;

	// ===== バトルの進行（今どの局面か）=====
	ImGui::Begin("GameFlow");
	ImGui::Text("State: %s", flow_.GetCurrentStateName());
	ImGui::End();

	// ===== Bloom 調整パネル（パーティクル層に掛かっているBloomを調整）=====
	if (auto* particleFx = PostEffectManager::GetInstance()->GetEffect(RenderLayerId::Particle)) {
		ImGui::Begin("Bloom (Particle)");
		if (ImGui::SliderFloat("Threshold", &bloomThreshold_, 0.0f, 1.0f)) {
			particleFx->SetBloomThreshold(bloomThreshold_);
		}
		if (ImGui::SliderFloat("Intensity", &bloomIntensity_, 0.0f, 8.0f)) {
			particleFx->SetBloomIntensity(bloomIntensity_);
		}
		if (ImGui::SliderInt("BlurIterations", &bloomIterations_, 1, 10)) {
			particleFx->SetBloomIterations(bloomIterations_);
		}
		ImGui::End();
	}
#endif
}

#include "TutorialScene.h"
#include "SceneManager.h"

void TutorialScene::Initialize()
{
	// ==============================================
	//    BaseSceneがLightを持っているため
	//    LightのInitialize()は必ず必要
	// ==============================================

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

	player_ = std::make_unique<Player>(this);

	followCamera = std::make_unique<FollowCamera>(player_.get(), 30.0f, 8.0f);
	followCamera->SetFarClip(2000.0f);

	player_->Initialize();
	player_->SetCamera(followCamera.get());

	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,0.0f,0.0f });

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });

	ground->SetCamera(followCamera.get());
	sky->SetCamera(followCamera.get());

	collisionManager_ = std::make_unique<CollisionManager>();

	// Actions：入力取得関数に合わせてここを書き換える
	TutorialController::Actions act{};
	// WASD入力が入っている間 true
	act.isMoving = []() -> bool {
		auto* in = Input::GetInstance();
		return in->PushKey(DIK_W) || in->PushKey(DIK_A) || in->PushKey(DIK_S) || in->PushKey(DIK_D);
		};

	// 押した瞬間だけ true（TriggerKeyがあるのでこれが一番安全）
	act.jumpTriggered = []() -> bool {
		return Input::GetInstance()->TriggerKey(DIK_SPACE);
		};

	// 左クリック（Triggerが無いのでエッジ検出を自前で作る）
	act.attackTriggered = []() -> bool {
		auto* in = Input::GetInstance();
		static bool prev = false;
		bool now = in->PushMouseButton(0);   // 0:左
		bool trig = (now && !prev);
		prev = now;
		return trig;
		};

	// Eキー（押した瞬間）
	act.rollTriggered = []() -> bool {
		return Input::GetInstance()->TriggerKey(DIK_E);
		};

	// 右クリック（Triggerが無いのでエッジ検出）
	act.dashTriggered = []() -> bool {
		auto* in = Input::GetInstance();
		static bool prev = false;
		bool now = in->PushMouseButton(1);   // 1:右
		bool trig = (now && !prev);
		prev = now;
		return trig;
		};

	// -----------------------------
	// Hooks: ガイド差し替え＆終了処理
	// -----------------------------
	TutorialController::Hooks hooks{};

	// ガイド画像が切り替わったらSpriteを差し替える
	hooks.onGuideChanged = [this](const std::string& path) {
		// 初回生成していないなら作る
		if (!guideSprite_) {
			guideSprite_ = std::make_unique<Sprite>();
			guideSprite_->Initialize(path);
			guideSprite_->SetAnchorPoint({ 0.5f, 0.5f });
			//guideSprite_->SetSize({ 900.0f, 160.0f });
			guideSprite_->SetPosition({ 640.0f, 120.0f });
			guideSprite_->SetColor({ 1,1,1,1 });
			guideSprite_->Update();
			currentGuidePath_ = path;
			return;
		}

		// 同じなら何もしない
		if (currentGuidePath_ == path) { return; }
		currentGuidePath_ = path;

		// テクスチャ差し替え（Initializeで差し替えできる前提）
		guideSprite_->Initialize(path);
		guideSprite_->SetAnchorPoint({ 0.5f, 0.5f });
		//guideSprite_->SetSize({ 900.0f, 160.0f });
		guideSprite_->SetPosition({ 640.0f, 120.0f });
		guideSprite_->SetColor({ 1,1,1,1 });
		guideSprite_->Update();
		};

	// チュートリアル終了 → GameSceneへ
	hooks.onFinished = []() {
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		};

	// Controller開始
	tutorial_.Initialize(act, hooks);
}

void TutorialScene::Finalize()
{
}

void TutorialScene::Update()
{

	// 各3Dオブジェクトの更新
	ground->Update();
	sky->Update();
	player_->Update();
	followCamera->Update();

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	tutorial_.Update(dt);

	// 衝突判定と応答
	CheckAllColisions();

	Debug();
}

void TutorialScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の背景描画
	// ================================================

	player_->BackGroundDraw();

	// ================================================
	// ここまでSprite個々の背景描画
	// ================================================
}

void TutorialScene::Draw()
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

void TutorialScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================

	player_->ForeGroundDraw();

	if (guideSprite_) {
		guideSprite_->Draw();
	}

	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================

	player_->ParticleDraw();

	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void TutorialScene::Debug()
{
#ifdef _DEBUG

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など




#endif
}

void TutorialScene::CheckAllColisions()
{
	collisionManager_->CheckAllCollisions();
}

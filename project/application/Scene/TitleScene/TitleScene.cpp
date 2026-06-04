#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <PostEffectManager.h>
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"

#ifdef USE_IMGUI
#include "externals/imgui/imnodes.h"
#endif

static float EaseOutCubic(float t) {
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	const float u = 1.0f - t;
	return 1.0f - (u * u * u);
}

void TitleScene::Initialize()
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

	// 3Dオブジェクトの初期化

	sneak = std::make_unique<Object3d>(this);
	sneak->Initialize();

	sword = std::make_unique<Object3d>(this);
	sword->Initialize();
	// sword ローカル調整（ボーン基準）
	swordTransform.scale = { 1.0f, 1.0f, 1.0f };
	swordTransform.rotate = { 0.0f, 0.0f, 0.0f };
	swordTransform.translate = { -0.6f, -1.1f, 0.0f };

	// 初期反映
	sword->SetScale(swordTransform.scale);
	sword->SetRotate(swordTransform.rotate);
	sword->SetTranslate(swordTransform.translate);

	// モデル読み込み
	ModelManager::GetInstance()->LoadModel("Warrior.gltf");
	ModelManager::GetInstance()->LoadModel("ground.obj");
	ModelManager::GetInstance()->LoadModel("skydome.obj");
	ModelManager::GetInstance()->LoadModel("sword.obj");
	sneak->SetModel("Warrior.gltf");
	sword->SetModel("sword.obj");


	// モデルにSRTを設定
	transform.scale = { 1,1,1 };
	transform.rotate = { 0.0f,3.14f,0.0f };
	transform.translate = { 0.0f,-1.0f,10.0f };
	sneak->SetTranslate(transform.translate);
	sneak->SetRotate(transform.rotate);
	sneak->SetScale(transform.scale);
	sneak->SetAnimation("Idle");



	ground = std::make_unique<Object3d>(this);
	ground->Initialize();
	ground->SetModel("ground.obj");
	ground->SetTranslate({ 0.0f,-2.0f,0.0f });
	ground->SetEnableLighting(false);

	sky = std::make_unique<Object3d>(this);
	sky->Initialize();
	sky->SetModel("skydome.obj");
	sky->SetTranslate({ 0.0f,0.0f,0.0f });


	// 3Dカメラの初期化
	camera1 = std::make_unique<Camera>();
	camera1->SetTranslate({ 0.0f, 1.0f, -20.0f });
	camera1->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera1->SetFarClip(2000.0f);
	// 初期角度を「今のカメラ位置」から合わせたい場合（任意）
	orbitAngle_ = 0.0f;
	orbitRadius_ = 40.0f;
	orbitHeight_ = 3.0f;
	orbitSpeed_ = -0.5f;


	// カメラのセット
	sneak->SetCamera(camera1.get());
	ground->SetCamera(camera1.get());
	sky->SetCamera(camera1.get());
	sword->SetCamera(camera1.get());

	//skybox->Initialize("Resources/rostock_laage_airport_4k.dds", { 1000.0f,1000.0f,1000.0f });
	skybox->SetCamera(camera1.get());

	// ===== タイトル表示位置（中央よりちょい上）=====
	// 画面中央よりちょい上
	titleCenterPos_ = {
		WinApp::kClientWidth * 0.5f,
		WinApp::kClientHeight * 0.25f
	};

	// 分割用（TitleCut）
	titleTop_ = std::make_unique<Sprite>();
	titleTop_->Initialize("Resources/Title.png");
	titleTop_->SetAnchorPoint({ 0.5f, 0.5f });

	titleBottom_ = std::make_unique<Sprite>();
	titleBottom_->Initialize("Resources/Title.png");
	titleBottom_->SetAnchorPoint({ 0.5f, 0.5f });

	// 画像サイズ取得（必ずメタデータから）
	const auto& md = TextureManager::GetInstance()->GetMetaData("Resources/Title.png");
	titleTexSize_ = { (float)md.width, (float)md.height };

	const float halfH = titleTexSize_.y * 0.5f;

	// 上半分（UV）
	titleTop_->SetTextureLeftTop({ 0.0f, 0.0f });
	titleTop_->SetTextureSize({ titleTexSize_.x, halfH });

	// 下半分（UV）
	titleBottom_->SetTextureLeftTop({ 0.0f, halfH });
	titleBottom_->SetTextureSize({ titleTexSize_.x, halfH });

	// ★ここが重要：表示サイズも半分にする（伸びなくなる）
	titleTop_->SetSize({ titleTexSize_.x, halfH });
	titleBottom_->SetSize({ titleTexSize_.x, halfH });

	// ★位置は ± halfH/2（= ±45）にする（今の ±22.5 は半分ズレ）
	titleTop_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f });
	titleBottom_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f });

	phase_ = TitlePhase::Idle;
	requestedShutter_ = false;
	titleCutTimer_ = 0.0f;
}

void TitleScene::Finalize()
{
}

void TitleScene::UpdateCamera()
{
	const Vector3 target = transform.translate + orbitTargetOffset_;

	const float x = target.x + std::sin(orbitAngle_) * orbitRadius_;
	const float z = target.z + std::cos(orbitAngle_) * orbitRadius_;
	const float y = target.y + orbitHeight_;

	camera1->SetTranslate({ x, y, z });

	Vector3 to = target - Vector3{ x, y, z };
	to = Normalize(to);

	const float yaw = std::atan2(to.x, to.z);
	const float pitch = std::atan2(-to.y, std::sqrt(to.x * to.x + to.z * to.z));

	camera1->SetRotate({ pitch, yaw, 0.0f });
	camera1->Update();

	if (ground) {
		ground->Update();
	}
	if (sky) {
		sky->Update();
	}
	if (sneak) {
		sneak->SetTranslate(transform.translate);
		sneak->SetRotate(transform.rotate);
		sneak->SetScale(transform.scale);
		sneak->Update();
	}
	if (sword) {
		sword->SetScale(swordTransform.scale);
		sword->SetRotate(swordTransform.rotate);
		sword->SetTranslate(swordTransform.translate);
		sword->SetParentJoint(sneak.get(), "Fist.R");
		sword->Update();
	}
	if (titleTop_) {
		titleTop_->Update();
	}
	if (titleBottom_) {
		titleBottom_->Update();
	}
}

void TitleScene::Update()
{
	// ===== タイトルスプライト更新（フェーズで切替）=====
	titleTop_->Update();
	titleBottom_->Update();

	// Δt（TimeManager があるならそっちを使うのがおすすめ）
	const float dt = TimeManager::GetInstance()->GetDeltaTime();

	// カメラの更新
	// ===== オービットカメラ：sneak を中心に回す =====
	{

		orbitAngle_ += orbitSpeed_ * dt;

		// 追従ターゲット（sneakの位置 + 少し上を見る）
		const Vector3 target = transform.translate + orbitTargetOffset_;

		// カメラ位置（XZで円運動）
		const float x = target.x + std::sin(orbitAngle_) * orbitRadius_;
		const float z = target.z + std::cos(orbitAngle_) * orbitRadius_;
		const float y = target.y + orbitHeight_;

		camera1->SetTranslate({ x, y, z });

		// カメラを常にターゲットへ向ける（LookAt的に回転を計算）
		// エンジンが rotate(yaw,pitch,roll) を使う前提で計算
		Vector3 to = target - Vector3{ x, y, z };
		to = Normalize(to);

		const float yaw = std::atan2(to.x, to.z);            // Y回転
		const float pitch = std::atan2(-to.y, std::sqrt(to.x * to.x + to.z * to.z)); // X回転

		camera1->SetRotate({ pitch, yaw, 0.0f });
	}

	camera1->Update();
	ground->Update();
	sky->Update();
	//skybox->Update();
	// 各3Dオブジェクトの更新
	sneak->SetTranslate(transform.translate);
	sneak->SetRotate(transform.rotate);
	sneak->SetScale(transform.scale);
	sneak->Update();

	// swordローカルオフセット反映
	sword->SetScale(swordTransform.scale);
	sword->SetRotate(swordTransform.rotate);
	sword->SetTranslate(swordTransform.translate);
	sword->SetParentJoint(sneak.get(), "Fist.R");
	sword->Update();

	// デバッグ
	Debug();

	// ===== 入力（Idleのときだけ）=====
	if (!SceneManager::GetInstance()->IsTransitioning()) {
		if (phase_ == TitlePhase::Idle) {
			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				StartMoveToFront();
			}
		}
	}

	// ===== フェーズ更新 =====
	if (phase_ == TitlePhase::MoveToFront) {
		UpdateMoveToFront(dt);
	}
	else if (phase_ == TitlePhase::AttackOnce) {
		UpdateAttackOnce(dt);
	}
	else if (phase_ == TitlePhase::TitleCut) {
		UpdateTitleCut(dt);
	}
}

void TitleScene::BackGroundDraw()
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

void TitleScene::Draw()
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
	sword->Draw();

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	sneak->Draw();

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

void TitleScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================

	// ===== タイトル描画（フェーズで切替）=====
	titleTop_->Draw();
	titleBottom_->Draw();



	// ================================================
	// ここまでSprite個々の前景描画(UIなど)
	// ================================================

	// ================================================
	// ここからparticle個々の描画
	// ================================================



	// ================================================
	// ここまでparticle個々の描画
	// ================================================
}

void TitleScene::Debug()
{
#ifdef USE_IMGUI

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など Scene UI
	//BaseScene::ShowFPS();

	ImGui::Begin("TitleScene Sword");

	ImGui::Text("Sword Local Transform (relative to parent joint)");
	ImGui::Separator();

	ImGui::DragFloat3("Translate", &swordTransform.translate.x, 0.01f);
	ImGui::DragFloat3("Rotate", &swordTransform.rotate.x, 0.01f);
	ImGui::DragFloat3("Scale", &swordTransform.scale.x, 0.01f);

	if (ImGui::Button("Reset")) {
		swordTransform.translate = { 0.0f, 0.0f, 0.0f };
		swordTransform.rotate = { 0.0f, 0.0f, 0.0f };
		swordTransform.scale = { 1.0f, 1.0f, 1.0f };
	}

	ImGui::End();

	// ================================================
	// imnodes テスト：ノードエディター
	// ================================================
	ImGui::Begin("Behavior Tree Editor (Test)");
	ImGui::Text("imnodes Test - Drag nodes, connect pins!");
	ImGui::Separator();

	ImNodes::BeginNodeEditor();

	// --- Root ノード ---
	ImNodes::BeginNode(1);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted("Root (Sequence)");
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(100);
	ImGui::Text("child 1 ->");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(101);
	ImGui::Text("child 2 ->");
	ImNodes::EndOutputAttribute();

	ImNodes::EndNode();

	// --- FindTarget ノード ---
	ImNodes::BeginNode(2);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted("FindTarget");
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginInputAttribute(200);
	ImGui::Text("-> in");
	ImNodes::EndInputAttribute();

	ImGui::Text("Detect player");

	ImNodes::EndNode();

	// --- ChargeDash ノード ---
	ImNodes::BeginNode(3);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted("ChargeDash");
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginInputAttribute(300);
	ImGui::Text("-> in");
	ImNodes::EndInputAttribute();

	// ★ ノード内に ImGui ウィジェットを埋め込める
	static float dashSpeed = 25.0f;
	static float chargeTime = 0.8f;
	ImGui::SetNextItemWidth(100.0f);
	ImGui::DragFloat("Dash Speed", &dashSpeed, 0.5f, 1.0f, 100.0f);
	ImGui::SetNextItemWidth(100.0f);
	ImGui::DragFloat("Charge Time", &chargeTime, 0.05f, 0.1f, 5.0f);

	ImNodes::EndNode();

	// --- リンク（ノード間の接続線）---
	// Link(linkId, startAttr, endAttr)
	ImNodes::Link(1, 100, 200);  // Root -> FindTarget
	ImNodes::Link(2, 101, 300);  // Root -> ChargeDash

	ImNodes::EndNodeEditor();

	// リンク作成の検出（ユーザーがドラッグで新しいリンクを作ったとき）
	int startAttr, endAttr;
	if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
		// ここで新しいリンクを保存する処理を書く（今はログだけ）
		OutputDebugStringA("New link created!\n");
	}

	ImGui::End();

#endif
}

void TitleScene::StartMoveToFront()
{
	phase_ = TitlePhase::MoveToFront;

	// キャラの正面方向（Yaw）
	const float yaw = transform.rotate.y;

	// forward（Z前方）
	const Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };

	// 正面からキャラを見る = カメラは target - forward*radius に置きたい
	// orbit式の角度へ変換
	desiredOrbitAngle_ = std::atan2(forward.x, forward.z);
}

void TitleScene::UpdateMoveToFront(float dt)
{
	// 目標角度へ最短で寄せる
	const float d = DeltaAngle(orbitAngle_, desiredOrbitAngle_);

	const float maxStep = orbitSpeedAbs_ * dt;
	float step = d;
	if (step > maxStep) step = maxStep;
	if (step < -maxStep) step = -maxStep;

	orbitAngle_ = NormalizeAngle(orbitAngle_ + step);

	// 充分近づいたら停止→Attackへ
	if (std::fabs(d) <= frontStopEps_) {
		orbitAngle_ = desiredOrbitAngle_;
		orbitSpeed_ = 0.0f; // 周回停止
		StartAttackOnce();
	}
}

void TitleScene::StartAttackOnce()
{
	phase_ = TitlePhase::AttackOnce;
	attackTimer_ = 0.0f;

	// 攻撃アニメへ
	sneak->SetAnimation("Attack02");
}

void TitleScene::UpdateAttackOnce(float dt)
{
	attackTimer_ += dt;

	// ここは実際のAttack02の長さに合わせて調整
	if (attackTimer_ >= attackDuration_) {
		sneak->SetAnimation("Idle");
		StartTitleCut();
	}
}

float TitleScene::NormalizeAngle(float a)
{
	while (a > 3.14159265f)  a -= 6.2831853f;
	while (a < -3.14159265f) a += 6.2831853f;
	return a;
}

float TitleScene::DeltaAngle(float from, float to)
{
	return NormalizeAngle(to - from);
}

void TitleScene::StartTitleCut()
{
	phase_ = TitlePhase::TitleCut;
	titleCutTimer_ = 0.0f;
	requestedShutter_ = false;

	// 分割スプライトを合体位置に戻す
	const float halfH = titleTexSize_.y * 0.5f;
	titleTop_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f });
	titleBottom_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f });
}

void TitleScene::UpdateTitleCut(float dt)
{
	titleCutTimer_ += dt;

	const float t = titleCutTimer_ / titleCutDuration_;
	const float e = EaseOutCubic(t);

	const float dx = titleCutDistance_ * e;
	const float dy = titleCutExtraY_ * e;

	const float halfH = titleTexSize_.y * 0.5f;
	const Vector2 topBase = { titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f };
	const Vector2 botBase = { titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f };

	// 上を左上、下を右下へ
	titleTop_->SetPosition({ topBase.x - dx, topBase.y - dy });
	titleBottom_->SetPosition({ botBase.x + dx, botBase.y + dy });

	// 終わったら Shutter → SceneChange
	if (!requestedShutter_ && titleCutTimer_ >= titleCutDuration_) {
		requestedShutter_ = true;

		PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);

		TransitionRequest req{};
		req.type = TransitionType::Shutter;
		req.fadeOutSec = 2.0f;
		req.fadeInSec = 2.5f;

		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}
}

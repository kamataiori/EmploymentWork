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

	// ===== スタート操作UI（Space ／ パッドA）=====
	// 2つを横並びにして画面中央下へ置く。色は乗せず、テクスチャのまま出す。
	startSpace_ = std::make_unique<Sprite>();
	startSpace_->Initialize("Resources/key_space_ui.png");
	startSpace_->SetAnchorPoint({ 0.5f, 0.5f });
	startSpace_->SetSize({ startUiSpaceW_, startUiSpaceH_ });

	startPadA_ = std::make_unique<Sprite>();
	startPadA_->Initialize("Resources/pad_a_ui.png");
	startPadA_->SetAnchorPoint({ 0.5f, 0.5f });
	startPadA_->SetSize({ startUiPadSize_, startUiPadSize_ });

	{
		// 2つ並べた全体幅を画面中央に合わせる
		const float totalW = startUiSpaceW_ + startUiGap_ + startUiPadSize_;
		const float leftX = WinApp::kClientWidth * 0.5f - totalW * 0.5f;
		const float y = WinApp::kClientHeight * startUiCenterYRatio_;
		startSpace_->SetPosition({ leftX + startUiSpaceW_ * 0.5f, y });
		startPadA_->SetPosition({ leftX + startUiSpaceW_ + startUiGap_ + startUiPadSize_ * 0.5f, y });
	}

	phase_ = TitlePhase::Idle;
	requestedChange_ = false;
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

	// スタート操作UIは Idle 中しか描かないので、その間だけ更新する
	if (phase_ == TitlePhase::Idle) {
		startSpace_->Update();
		startPadA_->Update();
	}

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
			if (Input::GetInstance()->TriggerKey(DIK_SPACE)
				|| Input::GetInstance()->TriggerButton(PadButton::A)) {
				StartMoveToFront();
			}
		}
	}

	// ===== フェーズ更新 =====
	if (phase_ == TitlePhase::MoveToFront) {
		UpdateMoveToFront(dt);
	}
	else if (phase_ == TitlePhase::SlashH) {
		UpdateSlashH(dt);
	}
	else if (phase_ == TitlePhase::SlashD) {
		UpdateSlashD(dt);
	}
	else if (phase_ == TitlePhase::GlassFall) {
		UpdateGlassFall(dt);
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

	// ===== スタート操作UI：押せる間（Idle）だけ出す =====
	// 出ている＝押せる。押した瞬間に斬撃演出へ入るので、そこからは消す。
	if (phase_ == TitlePhase::Idle) {
		startSpace_->Draw();
		startPadA_->Draw();
	}



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

	// 充分近づいたら停止→横斬りへ
	if (std::fabs(d) <= frontStopEps_) {
		orbitAngle_ = desiredOrbitAngle_;
		orbitSpeed_ = 0.0f; // 周回停止
		StartSlashH();
	}
}

void TitleScene::StartSlashH()
{
	phase_ = TitlePhase::SlashH;
	slashHTimer_ = 0.0f;
	requestedChange_ = false;

	// 横斬りアニメーション
	sneak->SetAnimation("Attack02");

	// タイトル文字を合体位置へ戻す
	const float halfH = titleTexSize_.y * 0.5f;
	titleTop_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f });
	titleBottom_->SetPosition({ titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f });

	// 画面全体（3Dシーン）を斬る SlashCut ポストエフェクトへ切替。
	// edge / hSep / dSep / slide。切れ目は細く、大きな動きは最後のガラス落下で出す。
	auto* pe = PostEffectManager::GetInstance();
	pe->SlashCutInitialize(0.006f, 0.02f, 0.02f, 0.08f);
	pe->SetSlashCutProgress(0.0f, 0.0f, 0.0f);
	pe->SetType(PostEffectType::SlashCut);
}

void TitleScene::UpdateSlashH(float dt)
{
	slashHTimer_ += dt;

	const float t = slashHTimer_ / slashHDuration_;
	const float e = EaseOutCubic(t);

	// タイトル文字：上を左上、下を右下へ（横斬り）
	const float dx = titleCutDistance_ * e;
	const float dy = titleCutExtraY_ * e;
	const float halfH = titleTexSize_.y * 0.5f;
	const Vector2 topBase = { titleCenterPos_.x, titleCenterPos_.y - halfH * 0.5f };
	const Vector2 botBase = { titleCenterPos_.x, titleCenterPos_.y + halfH * 0.5f };
	titleTop_->SetPosition({ topBase.x - dx, topBase.y - dy });
	titleBottom_->SetPosition({ botBase.x + dx, botBase.y + dy });

	// 画面全体：横のみ切断（斜め・落下はまだ）
	PostEffectManager::GetInstance()->SetSlashCutProgress(e, 0.0f, 0.0f);

	// 横斬り完了 → 斜め斬りへ
	if (slashHTimer_ >= slashHDuration_) {
		StartSlashD();
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

void TitleScene::StartSlashD()
{
	phase_ = TitlePhase::SlashD;
	slashDTimer_ = 0.0f;

	// 斜め斬りアニメーション
	sneak->SetAnimation("Attack03");
}

void TitleScene::UpdateSlashD(float dt)
{
	slashDTimer_ += dt;

	const float t = slashDTimer_ / slashDDuration_;
	const float e = EaseOutCubic(t);

	// 画面全体：横は切断済み(1.0)のまま、斜めを進める（落下はまだ）
	PostEffectManager::GetInstance()->SetSlashCutProgress(1.0f, e, 0.0f);

	// 斜め斬り完了 → ガラス落下へ
	if (slashDTimer_ >= slashDDuration_) {
		StartGlassFall();
	}
}

void TitleScene::StartGlassFall()
{
	phase_ = TitlePhase::GlassFall;
	fallTimer_ = 0.0f;

	sneak->SetAnimation("Idle");

	// 横斬り完了時のタイトル文字位置を落下開始点として記録
	const float halfH = titleTexSize_.y * 0.5f;
	titleTopFallBase_ = { titleCenterPos_.x - titleCutDistance_,
						  titleCenterPos_.y - halfH * 0.5f - titleCutExtraY_ };
	titleBottomFallBase_ = { titleCenterPos_.x + titleCutDistance_,
							titleCenterPos_.y + halfH * 0.5f + titleCutExtraY_ };
}

void TitleScene::UpdateGlassFall(float dt)
{
	fallTimer_ += dt;

	float f = fallTimer_ / fallDuration_;
	if (f > 1.0f) f = 1.0f;

	// 画面全体：横・斜めとも切断済み、ガラスとして落下（シェーダ側で重力 f^2）
	PostEffectManager::GetInstance()->SetSlashCutProgress(1.0f, 1.0f, f);

	// タイトル文字も割れたガラスと一緒に落下（重力 f^2＋左右へ少しドリフト）
	const float fallDist = WinApp::kClientHeight * 1.2f;
	const float g = f * f;
	titleTop_->SetPosition({ titleTopFallBase_.x - 30.0f * f, titleTopFallBase_.y + fallDist * g });
	titleBottom_->SetPosition({ titleBottomFallBase_.x + 30.0f * f, titleBottomFallBase_.y + fallDist * g });

	// 落下完了 → 本編へ。黒い幕(Shutter)は廃止。
	// ここでは SlashCut(fall=1＝真っ黒)のままにしておく。
	// Normal への復帰はシーン入れ替えの暗転点で SceneManager が自動で行うため、
	// ここで SetType(Normal) を呼ぶと fadeOut 中に元のタイトルが一瞬見えてしまう。
	if (!requestedChange_ && fallTimer_ >= fallDuration_) {
		requestedChange_ = true;

		TransitionRequest req{};
		req.type = TransitionType::Fade;
		req.fadeOutSec = 0.1f;
		req.fadeInSec = 1.2f;

		SceneManager::GetInstance()->RequestChangeScene("GAMEPLAY", req);
	}
}

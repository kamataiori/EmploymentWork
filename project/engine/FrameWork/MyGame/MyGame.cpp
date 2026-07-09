#include "MyGame.h"
#include "engine/TimeManager.h"

#ifdef USE_IMGUI
#include <externals/imgui/imgui_internal.h>
#endif // USE_IMGUI

void MyGame::Initialize()
{
	// 基底クラスの初期化処理
	Framework::Initialize();

	// シーンファクトリーを生成し、マネージャーにセット
	sceneFactory_ = new SceneFactory();
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_);

	// UIManager生成
	uiManager_ = std::make_unique<UIManager>();

	// 遷移サービス生成
	transitionService_ = std::make_unique<SceneTransitionService>();

	// 注入
	transitionService_->SetUIManager(uiManager_.get());
	SceneManager::GetInstance()->SetTransitionService(transitionService_.get());

	SceneManager::GetInstance()->ChangeScene("TITLE");

	// Play/Stop 状態を生成
	playModeState_ = std::make_unique<PlayModeState>();

	// Play 押下時: 現在のシーンを最初からリロードする
	playModeState_->SetOnPlayCallback([]() {
		SceneManager::GetInstance()->ReloadCurrentScene();
		});

	// SceneManager にも Play/Stop 状態を注入
	//    SceneManager::Update 内で IsPlaying を見て scene_->Update() をスキップする
	//    (ただしシーン切替直後の1フレームだけは強制的にUpdateを呼ぶ)
	SceneManager::GetInstance()->SetPlayModeState(playModeState_.get());

#ifdef USE_IMGUI
	// エディタ有効ビルドは停止状態から開始（Play ボタンで再生）
	playModeState_->InitializeStopped();
#else
	// エディタ無効ビルドは起動直後から再生状態
	playModeState_->InitializePlaying();
#endif

#ifdef USE_IMGUI
	// ImGuiManagerの初期化
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Initialize(winApp.get(), DirectXCommon::GetInstance());

	// EditorLayoutの初期化(ImGuiManagerの後に!)
	editorLayout_ = std::make_unique<EditorLayout>();
	editorLayout_->Initialize();

	// PlayModeState を EditorLayout に注入
	editorLayout_->SetPlayModeState(playModeState_.get());
#endif // USE_IMGUI

	PostEffectManager::GetInstance()->Initialize(PostEffectType::Normal);

	// パーティクル専用Canvasとレイヤー合成器を生成（Canvas方式のシーンで使用）
	// クリア色は透明（合成時にアルファブレンドで重ねるため）
	particleCanvas_ = std::make_unique<Canvas>();
	particleCanvas_->Initialize(DirectXCommon::GetInstance(),
		WinApp::kClientWidth, WinApp::kClientHeight, { 0.0f, 0.0f, 0.0f, 0.0f });
	layerCompositor_ = std::make_unique<LayerCompositor>();
	layerCompositor_->Initialize(DirectXCommon::GetInstance());

	// パーティクルCanvasのエフェクトスタックを Particle レイヤーとして登録簿へ注入。
	// これでシーンから PostEffectManager::SetLayerType(Particle, ...) 等で
	// パーティクル層だけに任意のポストエフェクトを掛けられる。
	PostEffectManager::GetInstance()->RegisterLayer(
		RenderLayerId::Particle, particleCanvas_->GetEffect());

	prevTime_ = std::chrono::steady_clock::now();

	GlobalVariables::GetInstance()->LoadFiles();
}

void MyGame::Finalize()
{
#ifdef USE_IMGUI
	// EditorLayoutを先に終了
	if (editorLayout_) {
		editorLayout_->SetPlayModeState(nullptr); // 参照を切る
		editorLayout_->Finalize();
	}
	editorLayout_.reset();

	// ImGuiの終了処理
	imGuiManager_->Finalize();
#endif // USE_IMGUI

	// SceneManager から PlayModeState 参照を外してから解放
	//    (ダングリング参照防止)
	SceneManager::GetInstance()->SetPlayModeState(nullptr);

	// Play/Stop 状態を解放
	playModeState_.reset();

	// 基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update()
{
#ifdef USE_IMGUI
	// ImGuiのフレーム開始
	imGuiManager_->Update();

	// BTエディタ専用シーン (ENEMYBT) では UE5風 EditorLayout を出さず、
	// BTエディタ画面だけを全画面で見せる
	const bool hideEditorLayout =
		(SceneManager::GetInstance()->GetCurrentSceneName() == "ENEMYBT");

	// UE5風レイアウト(DockSpace + 各パネル)を先に構築
	//    (再生中でも停止中でも毎フレーム動く)
	if (!hideEditorLayout) {
		editorLayout_->BeginFrame();
	}
#endif // USE_IMGUI

#ifdef USE_IMGUI
	// エディタ停止中はカーソルを強制解放する (シーン要求より優先)
	// → ImGui パネルを自由に操作できる。Play 押下で初めてシーン設定が反映される。
	if (cursorService_) {
		const bool stoppedInEditor = (playModeState_ && !playModeState_->IsPlaying());
		cursorService_->SetEditorOverride(stoppedInEditor);
	}
#endif // USE_IMGUI

	// 基底クラスの更新処理
	// (内部で SceneManager::Update も呼ばれ、そこで Play/Stop 判定される)
	Framework::Update();

	// --- UI更新は停止中はスキップ (UE5 Editor の完全凍結ルール) ---
	const bool isPlaying = (playModeState_ && playModeState_->IsPlaying());
	if (isPlaying) {
		uiManager_->Update();
	}

#ifdef USE_IMGUI
	// レイアウトの後処理 (BeginFrame を呼んだ時だけ EndFrame する)
	if (!hideEditorLayout) {
		editorLayout_->EndFrame();
	}

	// ImGuiの内部コマンドを生成
	ImGui::Render();
#endif // USE_IMGUI

	// ===== FPS & 時間計測 (常時動作) =====
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<float> delta = now - lastFrameTime_;
	lastFrameTime_ = now;

	float deltaSec = delta.count();
	if (deltaSec <= 0.0f) {
		deltaSec = 1.0f / 60.0f;
	}

	fps_ = 1.0f / deltaSec;
	frameTimeMs_ = deltaSec * 1000.0f;

	fpsHistory_.push_back(fps_);
	if (fpsHistory_.size() > kFpsHistorySize) {
		fpsHistory_.pop_front();
	}

	float sum = 0.0f;
	for (float f : fpsHistory_) sum += f;
	averageFps_ = sum / static_cast<float>(fpsHistory_.size());

#ifdef USE_IMGUI
	// ★ 計測したFPS統計をEditorLayoutに注入
	editorLayout_->SetPerformanceStats(fps_, frameTimeMs_, averageFps_);
#endif // USE_IMGUI

	// ===== TimeManager に生のΔtを渡す =====
	TimeManager::GetInstance()->Update(deltaSec);

	// ===== PostEffect にスケールなしΔtを渡す =====
	float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
	PostEffectManager::GetInstance()->RandomUpdate(unscaledDt);
}

void MyGame::Draw()
{
	// Lineのデータをリセット
	DrawLine::GetInstance()->ResetData();
	DrawTriangle::GetInstance()->ResetData();

	if (SceneManager::GetInstance()->CurrentSceneUsesCanvas()) {
		//========== Canvas合成方式 ==========//
		ID3D12GraphicsCommandList* cmd = dxCommon->GetCommandList().Get();

		// 1. World（背景＋3D＋前景スプライト）を既存オフスクリーン(scene RT)へ描画
		dxCommon->PreDrawForRenderTexture();
		SrvManager::GetInstance()->PreDraw();
		SceneManager::GetInstance()->DrawWorld();

		DrawLineCommon::GetInstance()->CommonSetting();
		DrawLine::GetInstance()->Draw();
		DrawTriangleCommon::GetInstance()->CommonSetting();
		DrawTriangle::GetInstance()->Draw();

		// 2. パーティクルを専用Canvasへ描画（深度は共有＝ワールドによる遮蔽を維持）
		particleCanvas_->BeginScene(cmd, /*clearDepth=*/false);
		SceneManager::GetInstance()->DrawParticles();
		particleCanvas_->EndScene(cmd);
		particleCanvas_->Resolve(cmd);

		// 3. バックバッファへ合成
		dxCommon->PreDraw();                    // バックバッファをバインド＋クリア
		dxCommon->PostDrawForRenderTexture();   // scene RT → SRV
		SrvManager::GetInstance()->PreDraw();

		// World（scene RT）にポストエフェクトを掛けてバックバッファへ
		PostEffectManager::GetInstance()->Draw();
		// パーティクルをその上に加算合成
		// （パーティクルはアルファを書き込まず「色×アルファ」がレイヤーに入るため、
		//   アルファ合成では消えてしまう。加算で重ねるのが正しい）
		layerCompositor_->Composite(cmd, particleCanvas_->GetResultSrvIndex(),
			LayerCompositor::BlendMode::Additive);

		// 4. UI（ポストエフェクト対象外）をバックバッファへ直接描画
		SpriteCommon::GetInstance()->CommonSetting();
		SceneManager::GetInstance()->DrawUI();
		uiManager_->Draw();
	}
	else {
		//========== 従来方式（単一オフスクリーン＋全体エフェクト）==========//
		// RenderTextureへの描画前処理
		dxCommon->PreDrawForRenderTexture();

		// RenderTexture用SRVの準備
		SrvManager::GetInstance()->PreDraw();

		// ゲームシーンの描画 (RenderTextureに対して)
		SceneManager::GetInstance()->Draw();

		// DrawLineの描画
		DrawLineCommon::GetInstance()->CommonSetting();
		DrawLine::GetInstance()->Draw();

		// DrawTriangleの描画
		DrawTriangleCommon::GetInstance()->CommonSetting();
		DrawTriangle::GetInstance()->Draw();

		SpriteCommon::GetInstance()->CommonSetting();
		uiManager_->Draw();

		// スワップチェーンへの描画前処理
		dxCommon->PreDraw();

		// RenderTextureの描画後処理
		dxCommon->PostDrawForRenderTexture();

		SrvManager::GetInstance()->PreDraw();

		PostEffectManager::GetInstance()->Draw();
	}

#ifdef USE_IMGUI
	// ImGuiの描画 (スワップチェーンに対して)
	imGuiManager_->Draw();
#endif // USE_IMGUI

	// スワップチェーンの描画後処理
	dxCommon->PostDraw();
}
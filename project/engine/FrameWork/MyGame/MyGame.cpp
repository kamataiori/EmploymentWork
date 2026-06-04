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

	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");

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

	// UE5風レイアウト(DockSpace + 各パネル)を先に構築
	//    (再生中でも停止中でも毎フレーム動く)
	editorLayout_->BeginFrame();
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
	// レイアウトの後処理
	editorLayout_->EndFrame();

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

#ifdef USE_IMGUI
	// ImGuiの描画 (スワップチェーンに対して)
	imGuiManager_->Draw();
#endif // USE_IMGUI

	// スワップチェーンの描画後処理
	dxCommon->PostDraw();
}
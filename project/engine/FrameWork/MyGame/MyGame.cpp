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

#ifdef USE_IMGUI
	// ImGuiManagerの初期化
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Initialize(winApp.get(), DirectXCommon::GetInstance());

	// ★ EditorLayoutの初期化（ImGuiManagerの後に！）
	editorLayout_ = std::make_unique<EditorLayout>();
	editorLayout_->Initialize();
#endif // USE_IMGUI

	PostEffectManager::GetInstance()->Initialize(PostEffectType::Normal);
	prevTime_ = std::chrono::steady_clock::now();

	GlobalVariables::GetInstance()->LoadFiles();
}

void MyGame::Finalize()
{
#ifdef USE_IMGUI
	// ★ EditorLayoutを先に終了
	if (editorLayout_) {
		editorLayout_->Finalize();
	}
	editorLayout_.reset();

	// ImGuiの終了処理
	imGuiManager_->Finalize();
#endif // USE_IMGUI

	// 基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update()
{
#ifdef USE_IMGUI
	// ImGuiのフレーム開始
	imGuiManager_->Update();

	// ★ UE5風レイアウト(DockSpace + 各パネル)を先に構築
	editorLayout_->BeginFrame();

	// ※ 以前ここにあった Performance ウィンドウは EditorLayout の
	//    Viewport オーバーレイ (Stat FPS) に移動したため削除

#endif // USE_IMGUI

	// 基底クラスの更新処理
	Framework::Update();

	// UI更新
	uiManager_->Update();

#ifdef USE_IMGUI
	// レイアウトの後処理
	editorLayout_->EndFrame();

	// ImGuiの内部コマンドを生成
	ImGui::Render();
#endif // USE_IMGUI

	// ===== FPS & 時間計測 =====
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
	// 計測したFPS統計をEditorLayoutに注入
	// 次フレームのStat FPSオーバーレイに反映される
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
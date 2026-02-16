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


	SceneManager::GetInstance()->ChangeScene("PARTICLE");

#ifdef USE_IMGUI

	// ImGuiManagerの初期化
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Initialize(winApp.get(), DirectXCommon::GetInstance());

#endif // USE_IMGUI

	//offscreenRendering->Initialize(PostEffectType::Normal);
	//postEffect->Initialize(PostEffectType::Normal);
	PostEffectManager::GetInstance()->Initialize(PostEffectType::Normal);
	prevTime_ = std::chrono::steady_clock::now();

	GlobalVariables::GetInstance()->LoadFiles();
}

void MyGame::Finalize()
{
#ifdef USE_IMGUI

	// ImGuiの終了処理
	imGuiManager_->Finalize();

#endif // USE_IMGUI

	// 基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update()
{
#ifdef USE_IMGUI

	// ImGuiのフレーム開始を宣言
	imGuiManager_->Update();
	ImGui::Begin("Performance");
	ImGui::Text("FPS: %.2f", GetFPS());
	ImGui::Text("Frame Time: %.2f ms", GetFrameTimeMs());
	ImGui::Text("Average FPS: %.2f", GetAverageFPS());
	ImGui::End();

#endif // USE_IMGUI

	// 基底クラスの更新処理
	Framework::Update();

	// UI更新（遷移演出含む）
	uiManager_->Update();

	//GlobalVariables::GetInstance()->Update();

#ifdef USE_IMGUI

	// ImGuiの内部コマンドを生成する
	ImGui::Render();

#endif // USE_IMGUI

	// ===== FPS & 時間計測 =====
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<float> delta = now - lastFrameTime_;
	lastFrameTime_ = now;

	float deltaSec = delta.count();
	if (deltaSec <= 0.0f) {
		deltaSec = 1.0f / 60.0f; // 万が一0になった時の保険
	}

	fps_ = 1.0f / deltaSec;
	frameTimeMs_ = deltaSec * 1000.0f;

	// 平均FPS更新
	fpsHistory_.push_back(fps_);
	if (fpsHistory_.size() > kFpsHistorySize) {
		fpsHistory_.pop_front();
	}

	float sum = 0.0f;
	for (float f : fpsHistory_) sum += f;
	averageFps_ = sum / static_cast<float>(fpsHistory_.size());



	// ===== TimeManager に生のΔtを渡す（ここが重要） =====
	TimeManager::GetInstance()->Update(deltaSec);

	// ===== PostEffect 側には「スケールなし」のΔtを渡す =====
	float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
	PostEffectManager::GetInstance()->RandomUpdate(unscaledDt);
}

void MyGame::Draw()
{
	// Lineのデータをリセット
	DrawLine::GetInstance()->ResetData();

	// Lineのデータをリセット
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

	//offscreenRendering->Draw();
	//postEffect->Draw();
	PostEffectManager::GetInstance()->Draw();

#ifdef USE_IMGUI

	// ImGuiの描画 (スワップチェーンに対して)
	imGuiManager_->Draw();

#endif // USE_IMGUI

	// スワップチェーンの描画後処理
	dxCommon->PostDraw();
}
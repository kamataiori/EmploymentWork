#include "MyGame.h"

void MyGame::Initialize()
{
	// 基底クラスの初期化処理
	Framework::Initialize();

	// シーンファクトリーを生成し、マネージャーにセット
	sceneFactory_ = new SceneFactory();
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_);

	SceneManager::GetInstance()->ChangeScene("TITLE");

	// ImGuiManagerの初期化
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Initialize(winApp.get(), DirectXCommon::GetInstance());

	offscreenRendering->Initialize();

	GlobalVariables::GetInstance()->LoadFiles();
}


void MyGame::Finalize()
{
	// ImGuiの終了処理
	imGuiManager_->Finalize();

	// 基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update()
{
	// ImGuiのフレーム開始を宣言
	imGuiManager_->Update();

	// 基底クラスの更新処理
	Framework::Update();

	GlobalVariables::GetInstance()->Update();

	// ImGuiの内部コマンドを生成する
	ImGui::Render();

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

	// スワップチェーンへの描画前処理
	dxCommon->PreDraw();

	// RenderTextureの描画後処理
	dxCommon->PostDrawForRenderTexture();

	SrvManager::GetInstance()->PreDraw();

	offscreenRendering->Draw();

	// ImGuiの描画 (スワップチェーンに対して)
	imGuiManager_->Draw();

	// スワップチェーンの描画後処理
	dxCommon->PostDraw();
}


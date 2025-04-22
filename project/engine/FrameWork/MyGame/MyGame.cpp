#include "MyGame.h"
#include <UnityScene.h>
#include <externals/imgui/imgui_internal.h>

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

	ApplyImGuiStyle();
#ifdef _DEBUG
	// Unity風レイアウトの表示
	if (useUnityLayout_) {
		DrawUnityLayout();
	}

	// レイアウト切り替えUI
	if (!useUnityLayout_) {
		ImGui::Begin("レイアウト切替");
		if (ImGui::Button("Unity風レイアウトに戻す")) {
			useUnityLayout_ = true;
			unityDockInitialized_ = false;
		}
		ImGui::End();
	}

	// Unityレイアウトの有効状態をSceneに通知
	auto* currentScene = SceneManager::GetInstance()->GetCurrentScene();
	if (auto* base = dynamic_cast<BaseScene*>(currentScene)) {
		base->SetEnableDockedImGui(useUnityLayout_);
	}
#endif


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

void MyGame::ApplyImGuiStyle() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_Header] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	colors[ImGuiCol_Button] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
}

void MyGame::DrawUnityLayout()
{
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpaceUnity", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("menu")) {
			if (ImGui::MenuItem("Unity風レイアウトを無効にする")) {
				useUnityLayout_ = false;
				unityDockInitialized_ = false;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");

	if (!unityDockInitialized_) {
		if (dockLayoutDelay_ < 1) {
			dockLayoutDelay_++;  // 1フレーム遅延させる
		}
		else {
			unityDockInitialized_ = true;
			dockLayoutDelay_ = 0;

			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->Size);

			ImGuiID dock_main_id = dockspaceID;
			ImGuiID dock_left, dock_right, dock_bottom, dock_center;
			ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, &dock_left, &dock_main_id);
			ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, &dock_right, &dock_center);
			ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_center);

			ImGuiID dock_left_top, dock_left_bottom;
			ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.6f, &dock_left_top, &dock_left_bottom);

			ImGuiID dock_right_top, dock_right_bottom;
			ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.6f, &dock_right_top, &dock_right_bottom);

			ImGui::DockBuilderDockWindow("SceneView", dock_center);
			ImGui::DockBuilderDockWindow("Hierarchy", dock_left_top);
			ImGui::DockBuilderDockWindow("Particle Control", dock_left_bottom);
			ImGui::DockBuilderDockWindow("Inspector", dock_right_top);
			ImGui::DockBuilderDockWindow("Debug Info", dock_right_bottom);
			ImGui::DockBuilderDockWindow("Project / Console", dock_bottom);

			// 🔻 TitleScene の登録ウィンドウもここでDock
			auto* currentScene = SceneManager::GetInstance()->GetCurrentScene();
			if (auto* base = dynamic_cast<BaseScene*>(currentScene)) {
				for (const auto& name : base->GetLeftDockWindows()) {
					ImGui::DockBuilderDockWindow(name.c_str(), dock_left_bottom);
				}
				for (const auto& name : base->GetRightDockWindows()) {
					ImGui::DockBuilderDockWindow(name.c_str(), dock_right_bottom);
				}
				for (const auto& name : base->GetBottomDockWindows()) {
					ImGui::DockBuilderDockWindow(name.c_str(), dock_bottom);
				}
			}

			ImGui::DockBuilderFinish(dockspaceID);
		}
	}


	ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	ImGui::End();

	// 分割した描画関数を呼び出す
	DrawCenterPanel();
	DrawLeftPanels();
	DrawRightPanels();
	DrawBottomPanel();
}

void MyGame::DrawCenterPanel()
{
	ImGui::Begin("SceneView");
	ImTextureID textureID = (ImTextureID)SrvManager::GetInstance()
		->GetGPUDescriptorHandle(offscreenRendering->GetSrvIndex()).ptr;
	ImGui::Image(textureID, ImGui::GetContentRegionAvail());
	ImGui::End();
}

void MyGame::DrawLeftPanels()
{

	if (!useUnityLayout_) return;

	/*ImGui::Begin("Hierarchy");
	ImGui::Text("Hierarchy内容");
	ImGui::End();

	ImGui::Begin("Particle Control");
	ImGui::Text("パーティクル制御");
	ImGui::End();*/
}

void MyGame::DrawRightPanels()
{
	if (!useUnityLayout_) return;

	/*ImGui::Begin("Inspector");
	ImGui::Text("インスペクター表示");
	ImGui::End();

	ImGui::Begin("Debug Info");
	ImGui::Text("デバッグ情報やFPS");
	ImGui::End();*/
}

void MyGame::DrawBottomPanel()
{
	if (!useUnityLayout_) return;

	/*ImGui::Begin("Project / Console");
	ImGui::Text("プロジェクト・コンソール表示");
	ImGui::End();*/
}











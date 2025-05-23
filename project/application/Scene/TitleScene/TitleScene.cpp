#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include <PostEffectManager.h>

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

	// 背景のSprite初期化
	backGround_->Initialize("Resources/pokertable.png");

	texasHoldem_.Initialize();
} 

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{

	backGround_->Update();

	// スペースキーが押された瞬間かつ前のフレームでは押されていなかった場合のみ実行
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		if (!isPressed_) {
			if (texasHoldem_.GetCurrentPhase() == Phase::Showdown) {
				texasHoldem_.Reset(); // リスタート！
			}
			else {
				texasHoldem_.NextPhase(); // 通常進行
			}
			isPressed_ = true;
		}
	}
	else {
		isPressed_ = false;
	}

	// プレイヤーの役を常に表示（左上）
	ImGui::Begin("PlayerHandInfo", nullptr,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::SetWindowPos(ImVec2(10, 10));
	ImGui::SetWindowSize(ImVec2(200, 50));
	ImGui::Text("Player: %s", HandRankToString(texasHoldem_.GetCurrentHandRank()));
	ImGui::End();

	// CPUの役を Showdown のときだけ表示（右上）
	if (texasHoldem_.GetCurrentPhase() == Phase::Showdown) {
		// 中央上に勝者表示
		ImGui::Begin("WinnerDisplay", nullptr,
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowPos(ImVec2(540, 200)); // 中央寄せ（1280px幅想定）
		ImGui::SetWindowSize(ImVec2(600, 100));
		ImGui::Text("%s", texasHoldem_.GetWinnerName().c_str());
		ImGui::End();
	}

	texasHoldem_.Update();


}

void TitleScene::BackGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の背景描画
	// ================================================

	backGround_->Draw();

	// ================================================
	// ここまでSprite個々の背景描画
	// ================================================
}

void TitleScene::Draw()
{
	

	// 3Dオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Object3dCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここから3Dオブジェクト個々の描画
	// ================================================

	// 各オブジェクトの描画
	

	// ================================================
	// ここまで3Dオブジェクト個々の描画
	// ================================================

	//	アニメーションオブジェクトの描画前処理。3Dオブジェクトの描画設定に共通のグラフィックスコマンドを積む
	Skinning::GetInstance()->CommonSetting();

	// ================================================
	// ここからアニメーションオブジェクトの個々の描画
	// ================================================

	// 各オブジェクトの描画
	

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

	// カード描画
	texasHoldem_.Draw();


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
#ifdef _DEBUG

	
#endif
}

#include "MenuScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "PostEffectManager.h"
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"

void MenuScene::Initialize()
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
}

void MenuScene::Finalize()
{
}

void MenuScene::Update()
{
	// 遷移中でなければ入力受付
	if (!SceneManager::GetInstance()->IsTransitioning()) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)
			|| Input::GetInstance()->TriggerButton(PadButton::A)) {
			PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);

			/*TransitionRequest req{};
			req.type = TransitionType::Fade;
			req.fadeOutSec = 2.0f;
			req.fadeInSec = 1.0f;*/

			TransitionRequest req{};
			req.type = TransitionType::Shutter;
			req.fadeOutSec = 2.0f;  // 閉じる
			req.fadeInSec = 2.5f;  // 開く

			SceneManager::GetInstance()->RequestChangeScene("TUTORIAL", req);

		}
	}
}

void MenuScene::BackGroundDraw()
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

void MenuScene::Draw()
{
	//skybox->Draw();

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

void MenuScene::ForeGroundDraw()
{
	// Spriteの描画前処理。Spriteの描画設定に共通のグラフィックスコマンドを積む
	SpriteCommon::GetInstance()->CommonSetting();

	// ================================================
	// ここからSprite個々の前景描画(UIなど)
	// ================================================




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

void MenuScene::Debug()
{
#ifdef USE_IMGUI

	if (!IsDockedImGuiEnabled()) return;

	// ↓ ここから ImGui::Begin(...) など Scene UI
	//BaseScene::ShowFPS();



#endif
}

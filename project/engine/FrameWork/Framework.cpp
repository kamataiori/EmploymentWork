#include "Framework.h"

void Framework::Initialize()
{
	// WindowsAPIの初期化
	winApp = std::make_unique<WinApp>();
	winApp->Initialize();

	// 入力の初期化
	Input::GetInstance()->Initialize(winApp.get());

	// オーディオの初期化
	audio = std::make_unique<Audio>();
	audio->Initialize();

	// DirectXの初期化
	dxCommon = DirectXCommon::GetInstance();
	dxCommon->Initialize(winApp.get());

	// SrvManagerの初期化
	//SrvManager::GetInstance()->Initialize();

	// Sprite共通部の初期化
	SpriteCommon::GetInstance()->Initialize();

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize();

	// 3Dオブジェクト共通部の初期化
	Object3dCommon::GetInstance()->Initialize();

	// スキニング共通部の初期化
	Skinning::GetInstance()->Initialize();

	// モデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// モデル共通部の初期化
	modelCommon = std::make_unique<ModelCommon>();
	modelCommon->Initialize();

	// DrawLineの初期化
	DrawLineCommon::GetInstance()->Initialize();
	DrawLine::GetInstance()->Initialize();

	// DrawTriangleの初期化
	DrawTriangleCommon::GetInstance()->Initialize();
	DrawTriangle::GetInstance()->Initialize();

}

void Framework::Finalize()
{
	// DirectXの解放
	CloseHandle(dxCommon->GetFenceEvent());

	// WinAppの終了処理
	winApp->Finalize();
	// 各クラスの解放
	Input::GetInstance()->Finalize();
	// SrvManagerの終了処理
	SrvManager::GetInstance()->Finalize();
	// テクスチャマネージャーの終了処理
	TextureManager::GetInstance()->Finalize();
	// モデルマネージャーの終了処理
	ModelManager::GetInstance()->Finalize();
	// シーンマネージャーの終了処理
	SceneManager::GetInstance()->Finalize();
	// DrawLineの終了処理
	DrawLineCommon::GetInstance()->Finalize();
	// DrawLineの終了処理
	DrawLine::GetInstance()->Finalize();
	// DrawTriangleの終了処理
	DrawTriangleCommon::GetInstance()->Finalize();
	// DrawTriangleの終了処理
	DrawTriangle::GetInstance()->Finalize();
	delete sceneFactory_;
	spriteCommon.reset();
	spriteCommon->Finalize();
	object3dCommon.reset();
	object3dCommon->Finalize();
	modelCommon.reset();
	skinningCommon.reset();
	skinningCommon->Finalize();
	dxCommon->Finalize();
}

void Framework::Update()
{
	// Windowsにメッセージが来てたら最優先で処理させる
	if (winApp->ProcessMesage()) {
		endRequest_ = true;
	}

	// シーンマネージャーの更新処理
	SceneManager::GetInstance()->Update();

	// 入力の更新
	Input::GetInstance()->Update();

	// DrawLineの更新
	DrawLine::GetInstance()->Update();

	// DrawTriangleの更新
	DrawTriangle::GetInstance()->Update();
}

void Framework::Run()
{
	// ゲームの初期化
	Initialize();

	while (true) // ゲームループ
	{
		// 毎フレーム更新
		Update();
		//終了リクエストが来たら抜ける
		if (IsEndRequest()) {
			break;
		}
		//描画
		Draw();
	}
	//ゲームの終了
	Finalize();
}

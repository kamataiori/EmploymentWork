#pragma once
#include <memory>
#include <WinApp.h>
#include <Input.h>
#include <DirectXCommon.h>
#include <SpriteCommon.h>
#include <Object3dCommon.h>
#include <ModelCommon.h>
#include "SrvManager.h"
#include "SceneManager.h"
#include "AbstractSceneFactory.h"
#include "Audio.h"
#include "D3DResourceLeakChecker.h"
#include "DrawLineCommon.h"
#include <Debug.h>
#include "DrawTriangleCommon.h"
#include <DrawTriangle.h>
#include <Skinning.h>

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

class Framework
{
public:
	//------メンバ関数------//

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw() = 0;

	/// <summary>
	/// 終了チェック
	/// </summary>
	virtual bool IsEndRequest() { return endRequest_; }

	/// <summary>
	/// 実行
	/// </summary>
	void Run();

public:
	//------メンバ変数------//

	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~Framework() = default;

protected:
	// WinAppの初期化
	std::unique_ptr<WinApp> winApp = nullptr;
	// DxCommonの初期化
	DirectXCommon* dxCommon = nullptr;
	// Audioの初期化
	std::unique_ptr<Audio> audio = nullptr;
	// SrvManagerの初期化
	std::unique_ptr<SrvManager> srvManager = nullptr;
	// Sprite共通部の初期化
	std::unique_ptr<SpriteCommon> spriteCommon = nullptr;
	// 3Dオブジェクト共通部の初期化
	std::unique_ptr<Object3dCommon> object3dCommon = nullptr;
	// モデル共通部の初期化
	std::unique_ptr<ModelCommon> modelCommon = nullptr;
	// スキニング共通部の初期化
	std::unique_ptr<Skinning> skinningCommon = nullptr;

	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;


private:
	// ゲーム終了フラグ
	bool endRequest_ = false;

};


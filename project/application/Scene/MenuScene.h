#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "Object3d.h"
#include "MenuInput.h"
#include <memory>

class MenuScene : public BaseScene
{
public:
	//------メンバ関数------

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 背景描画
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景描画
	/// </summary>
	void ForeGroundDraw() override;

	void Debug() override;

private:

	// このシーンの入力。操作はメニュー操作一式なので共通の MenuInput を使う。
	std::unique_ptr<MenuInput> sceneInput_ = nullptr;
};


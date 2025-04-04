#pragma once
#include "BaseScene.h"
#include <string>
#include "TitleScene.h"
#include "GamePlayScene.h"

class AbstractSceneFactory
{
public:
	// 仮想デストラクタ
	virtual ~AbstractSceneFactory() = default;

	// シーン生成
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;

};


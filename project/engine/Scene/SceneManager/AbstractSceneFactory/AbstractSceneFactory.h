#pragma once
#include "BaseScene.h"
#include <string>
//#include "TitleScene.h"
//#include "GamePlayScene.h"
//#include "UnityScene.h"

class AbstractSceneFactory
{
public:
	// 仮想デストラクタ
	virtual ~AbstractSceneFactory() = default;

	// シーン生成
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;

	/// <summary>
	/// 登録されているシーン名の一覧を取得
	/// UIでのシーン切り替えなどに使用
	/// </summary>
	virtual std::vector<std::string> GetSceneNameList() const = 0;

};


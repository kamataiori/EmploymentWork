#pragma once
#include "AbstractSceneFactory.h"

class SceneFactory : public AbstractSceneFactory
{
public:

	//------シーン生成------
	BaseScene* CreateScene(const std::string& sceneName) override;

	/// <summary>
	/// 登録されているシーン名の一覧
	/// 新しいシーンを追加したら、この配列にも追加すること
	/// </summary>
	std::vector<std::string> GetSceneNameList() const override;
};


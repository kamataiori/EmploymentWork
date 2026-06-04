#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "ParticleEditorScene.h"
#include "application/Scene/TutorialScene/TutorialScene.h"
#include "application/Scene/EditorScene/BTEditorScene.h"
#include "MenuScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
	// 次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE")
	{
		newScene = new TitleScene();
	}
	else if (sceneName == "GAMEPLAY")
	{
		newScene = new GamePlayScene();
	}
	else if (sceneName == "PARTICLE")
	{
		newScene = new ParticleEditorScene();
	}
	else if (sceneName == "TUTORIAL")
	{
		newScene = new TutorialScene();
	}
	else if (sceneName == "MENU")
	{
		newScene = new MenuScene();
	}
	else if (sceneName == "ENEMYBT")
	{
		newScene = new BTEditorScene();
	}
	
	return newScene;
}

std::vector<std::string> SceneFactory::GetSceneNameList() const
{
	// CreateScene の if 分岐と同じ順で並べる
	// シーンを追加したら、ここにも追加すること
	return {
		"TITLE",
		"GAMEPLAY",
		"PARTICLE",
		"TUTORIAL",
		"MENU",
		"ENEMYBT",
	};
}

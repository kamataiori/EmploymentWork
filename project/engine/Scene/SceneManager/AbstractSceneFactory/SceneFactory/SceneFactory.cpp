#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "ParticleEditorScene.h"
#include "application/Scene/TutorialScene/TutorialScene.h"

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
	
	return newScene;
}

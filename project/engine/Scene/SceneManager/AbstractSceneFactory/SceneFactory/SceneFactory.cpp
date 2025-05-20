#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "UnityScene.h"
#include "ParticleScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
	// 次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
	}
	else if (sceneName == "GAMEPLAY") {
		newScene = new GamePlayScene();
	}
	else if (sceneName == "Unity") {
		newScene = new UnityScene();
	}
	else if (sceneName == "PARTICLE") {
		newScene = new ParticleScene();
	}
	
	return newScene;
}
